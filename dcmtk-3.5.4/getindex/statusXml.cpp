#include "stdafx.h"
#include <lock.h>
#include <liblock.h>
#include <libb24.h>
#include <gencard.h>
#import <msxml3.dll>
using namespace std;

static long __stdcall Lock32_Function_Wrapper(long randnum)
{
	long tr = 0;
	Lock32_Function(randnum, &tr, 0);
	return tr;
}

int statusCharge(const char *flag)
{
	string errorMessage;
	MSXML2::IXMLDOMDocumentPtr pXmlDom;
	HRESULT hr = pXmlDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		index_errlog << "Failed to CreateInstance on an XML DOM." << endl;
		outputContent(true);
		return -1;
	}
	pXmlDom->preserveWhiteSpace = VARIANT_FALSE;
	pXmlDom->async = VARIANT_FALSE;

	MSXML2::IXMLDOMProcessingInstructionPtr pi = pXmlDom->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"gbk\"");
	if (pi != NULL) pXmlDom->appendChild(pi);

	MSXML2::IXMLDOMProcessingInstructionPtr pXslt = NULL;
	pXslt = pXmlDom->createProcessingInstruction("xml-stylesheet", "type=\"text/xml\" href=\"../xslt/charge.xsl\"");
	if (pXslt != NULL) pXmlDom->appendChild(pXslt);
	
	MSXML2::IXMLDOMElementPtr root = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "charge_status", "");
	pXmlDom->appendChild(root);
	MSXML2::IXMLDOMElementPtr errorInfos = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_infos", "");

	int licenseCount = 0, oldCount = -1;
	WORD increase = 0;
	char countBuffer[12] = "", passwd[9] = "", filename[64] = "..\\etc\\*.key";
	int lockNumber = getLockNumber(filename, FALSE, filename + 7, 64 - 7);
	SEED_SIV siv;

	MSXML2::IXMLDOMElementPtr key;
	key = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "key", "");
	// charge?
	char chargekey[24] = "";
	if(flag && strlen(flag) && !strcmp("charge", flag)
		&& cgiFormNotFound != cgiFormString("password", chargekey, sizeof(chargekey)) && strlen(chargekey) > 0)
	{
		char seqStr[16] = "";
		int seq = -1;
		if(cgiFormNotFound != cgiFormString("seq", seqStr, sizeof(seqStr)) && strlen(seqStr) > 0)
		{
			errno = 0;
			seq = atoi(seqStr);
		}
		if(seq == -1 || (seq == 0 && errno == EINVAL))
		{
			index_errlog << "ÐòÁÐºÅ´íÎó" << endl;
			outputContent(true);
			return -1;
		}
#ifdef NDEBUG
		Sleep(4000);
#endif
		DWORD serial = 0;
		if(!SetLock(8, &serial, 0, "fqE8km*O", "Tw2d@uJp", 0, 0))
		{
			index_errlog << "»ñÈ¡¼ÓÃÜËøÐòºÅ´íÎó:" << hex << LYFGetLastErr() << endl;
			outputContent(true);
			return -2;
		}
#ifdef PACS_ENABLE_CHARGE
		long salt = serial;
		BOOL lockResult = TRUE;
		lockResult = lockResult && Lock32_Function(salt, &salt, 0);
		for(int i = 0; i < (seq % 13); ++i)
			lockResult = lockResult && Lock32_Function(salt, &salt, 0);
		if(!lockResult)
		{
			index_errlog << "¼ÓÃÜËøÐ£Ñé´íÎó" << endl;
			outputContent(true);
			return -2;
		}
		
		// check batch mode flag
		bool isBatchMode = true;
		if(0 == loadPublicKeyContentRW(filename, &siv, lockNumber, passwd))
		{
			DWORD data = 0;
			if(ReadLock(MODE_FLAG_POS, &data, passwd, 0, 0))
			{
				 isBatchMode = (1 == (data & 1));
			}
		}
		if(isBatchMode)
		{
			index_errlog << "ÊýÁ¿»òÐòÁÐºÅ´íÎó: " << chargekey << endl;
			outputContent(true);
			return -16;
		}

		int retCode = decodeCharge(chargekey, salt, Lock32_Function_Wrapper);
		switch(retCode)
		{
		case -1:
			index_errlog << "´íÎó1:" << chargekey << endl;
			outputContent(true);
			return -3;
		case -2:
			index_errlog << "´íÎó2:" << chargekey << endl;
			outputContent(true);
			return -4;
		case -3:
			index_errlog << "´íÎó3:" << chargekey << endl;
			outputContent(true);
			return -5;
		}
		DWORD box = ((unsigned int)retCode) >> 24;
		DWORD fileno = retCode & 0xFFFF;
		if(box > MAX_BOX || fileno != seq || fileno >= TOTAL_BUY)
		{
			index_errlog << "ÊýÁ¿»òÐòÁÐºÅ´íÎó:" << chargekey << endl;
			outputContent(true);
			return -6;
		}

		if(lockNumber != -1 && 0 == loadPublicKeyContent2Pwd(filename, &siv, lockNumber, passwd, NULL))
		{
			if(!invalidLock("..\\etc\\license.key", filename, &siv))
			{
				licenseCount = currentCount(passwd);
				if(licenseCount < 0 || licenseCount > 0xffff)
				{
					index_errlog << "ÊÚÈ¨´íÎó" << endl;
					outputContent(true);
					return -1;
				}
			}
			else
			{
				index_errlog << "ÊÚÈ¨´íÎó" << endl;
				outputContent(true);
				return -1;
			}
		}
		else
		{
			index_errlog << "ÊÚÈ¨´íÎó" << endl;
			outputContent(true);
			return -1;
		}

		char timeBuffer[16];
		GenerateTime(DATE_FORMAT_COMPACT, timeBuffer, sizeof(timeBuffer));
		ofstream chargeLog("pacs_log\\charge.log", ios_base::app, _SH_DENYRW);
		if(!chargeLog.fail())
		{
			chargeLog << chargekey << '\t' << timeBuffer << '\t';
			int sectionNumber = fileno / 32 + CHARGE_BASE, offset = (fileno % 32) / 8;
			unsigned char bytemask = 0x80, section[4];
			bytemask >>= (fileno % 8);
			if(ReadLock(sectionNumber, section, passwd, 0, 0))
			{
				if((section[offset] & bytemask) == 0)
				{
					section[offset] |= bytemask;

					increase = box * 50;
					oldCount = licenseCount;
					if(licenseCount >= 0 && (licenseCount + increase) <= MAX_MEDIA_COUNT)
					{
						licenseCount = increaseCount(passwd, increase);
						if(licenseCount >= 0)
						{
							if(WriteLock(sectionNumber, section, passwd, 0, 0))
								chargeLog << "OK:" << increase << endl;
							else  // rollback
							{
								licenseCount = increaseCount(passwd, -increase);
								errorMessage = "´æ´¢Ð´Èë´íÎó";
							}
						}
						else
							errorMessage = "ÊýÁ¿Ð´Èë´íÎó";
					}
					else
					{
						stringstream ss("³äÖµÊýÁ¿²»ÄÜ³¬¹ý");
						ss << MAX_MEDIA_COUNT;
						errorMessage = ss.str();
					}
				}
				else
					errorMessage = "´ËÃÜÂëÒÑ³ä¹ý";
			}
			else
				errorMessage = "´æ´¢¶ÁÈ¡´íÎó";
			if(errorMessage.length() > 0) chargeLog << "error:" << errorMessage << ',' << hex << LYFGetLastErr() << endl;
			chargeLog.close();
		}
		else
		{
			index_errlog << "³äÖµÈÕÖ¾Ð´Èë´íÎó£¬ÇëÖØÊÔ" << endl;
			outputContent(true);
			return -9;
		}
#else
		// dummy code, must fail
		DWORD tmp;
		if(!SetLock(1, (unsigned long*)&seq, 0, chargekey, "Tw2d@uJp", 0, 0))
		{
			buffer << "ÊýÁ¿»òÐòÁÐºÅ´íÎó " << seq << ":" << chargekey << endl;
			outputContent(true);
			return -9;
		}
		if(!ReadLock(300, &tmp, "fqE8km*O", 0, 0))
		{
			buffer << "ÊýÁ¿»òÐòÁÐºÅ´íÎó " << seq << ":" << chargekey << endl;
			outputContent(true);
			return -9;
		}
#endif
		key->appendChild(pXmlDom->createTextNode(chargekey));
		root->appendChild(key);
	}
	else  // query counter
	{
		if(0 == loadPublicKeyContentRW(filename, &siv, lockNumber, passwd))
		{
			if(!invalidLock("..\\etc\\license.key", filename, &siv))
			{
				licenseCount = currentCount(passwd);
				if(licenseCount < 0 || licenseCount > 0xFFFF)
				{
					index_errlog << "ÊÚÈ¨´íÎó" << endl;
					outputContent(true);
					return -1;
				}
			}
			else
			{
				index_errlog << "ÊÚÈ¨´íÎó" << endl;
				outputContent(true);
				return -1;
			}
		}
		else
		{
			index_errlog << "ÊÚÈ¨´íÎó" << endl;
			outputContent(true);
			return -1;
		}
	}

	sprintf_s(countBuffer, "%d", lockNumber);
	MSXML2::IXMLDOMElementPtr lockName;
	lockName = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "lock_number", "");
	lockName->appendChild(pXmlDom->createTextNode(countBuffer));
	root->appendChild(lockName);

	sprintf_s(countBuffer, "%d", licenseCount);
	MSXML2::IXMLDOMElementPtr counter;
	counter = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "license_counter", "");
	counter->appendChild(pXmlDom->createTextNode(countBuffer));
	root->appendChild(counter);

	if(increase > 0)
	{
		sprintf_s(countBuffer, "%d", increase);
		MSXML2::IXMLDOMElementPtr increaseptr;
		increaseptr = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "increase", "");
		increaseptr->appendChild(pXmlDom->createTextNode(countBuffer));
		root->appendChild(increaseptr);
	}

	if(oldCount != -1)
	{
		sprintf_s(countBuffer, "%d", oldCount);
		MSXML2::IXMLDOMElementPtr oldcounter;
		oldcounter = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "old_counter", "");
		oldcounter->appendChild(pXmlDom->createTextNode(countBuffer));
		root->appendChild(oldcounter);
	}

	if(errorMessage.length() > 0)
	{
		errorInfos->appendChild(pXmlDom->createTextNode(errorMessage.c_str()));
		root->appendChild(errorInfos);
	}
	index_errlog << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << (pXslt ? pXslt->xml : "") << root->xml;
	outputContent(false);
	return 0;
}

int removeStudy(const char *flag)
{
	ostringstream errorMessageStream;
	char studyUID[65], patientID[65], indexPath[MAX_PATH], studyDateBuf[16], receiveDateBuf[16], mode[16];
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		char hashBuf[9];
		__int64 hashStudy = uidHash(studyUID, hashBuf, sizeof(hashBuf));
		sprintf_s(indexPath, MAX_PATH, "archdir\\%c%c\\%c%c\\%c%c\\%c%c\\%s",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
		if(deleteTree(indexPath, &errorMessageStream))
		{
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.xml",
				hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
			int rmXml = remove(indexPath);
			if(rmXml && errno == ENOENT) rmXml = 0;
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c\\%s.txt",
				hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUID);
			int rmTxt = remove(indexPath);
			if(rmTxt && errno == ENOENT) rmTxt = 0;
			if(rmXml || rmTxt)
				errorMessageStream << "¼ì²éÍ¼ÏñÉ¾³ý³É¹¦, ¼ì²éË÷ÒýÉ¾³ýÊ§°Ü" << endl;
		}
		else
			errorMessageStream << "¼ì²éÉ¾³ýÊ§°Ü" << endl;
	}
	else
    {
		errorMessageStream << "¼ì²éUID´íÎó" << endl;
        goto remove_study_study_uid_error;
    }

    if(cgiFormNotFound == cgiFormString("mode", mode, sizeof(mode))) mode[0] = '\0';

	if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
	{
		if(!deleteStudyFromIndex("00100020", patientID, studyUID))
			errorMessageStream << "»¼ÕßË÷ÒýÉ¾³ý´íÎó" << endl;
	}
	else if(strcmp(mode, "00100020") == 0)
		errorMessageStream << "»¼ÕßID´íÎó" << endl;

    if(cgiFormNotFound != cgiFormString("receiveDate", receiveDateBuf, sizeof(receiveDateBuf)) && strlen(receiveDateBuf) > 0)
    {
		if(!deleteStudyFromIndex("receive", receiveDateBuf, studyUID))
			errorMessageStream << "´«ÊäÈÕÆÚË÷ÒýÉ¾³ý´íÎó" << endl;
	}
	else if(strcmp(mode, "receive") == 0)
		errorMessageStream << "´«ÊäÈÕÆÚ´íÎó" << endl;
    
    if(cgiFormNotFound != cgiFormString("studyDate", studyDateBuf, sizeof(studyDateBuf)) && strlen(studyDateBuf) > 0)
    {
		if(!deleteStudyFromIndex("00080020", studyDateBuf, studyUID))
			errorMessageStream << "¼ì²éÈÕÆÚË÷ÒýÉ¾³ý´íÎó" << endl;
	}
	else if(strcmp(mode, "00080020") == 0)
		errorMessageStream << "¼ì²éÈÕÆÚ´íÎó" << endl;

remove_study_study_uid_error:
    string errorMessage = errorMessageStream.str();
    if(errorMessage.empty()) goto remove_study_no_error;
remove_study_error:
	fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", errorMessage.size());
	fprintf(cgiOut, errorMessage.c_str());
    return 0;
remove_study_no_error:
    if(strcmp(mode, "00100020") == 0)
        sprintf_s(indexPath, "../index.htm?mode=00100020&patientID=%s", patientID);
    else if(strcmp(mode, "00080020") == 0)
        sprintf_s(indexPath, "../index.htm?mode=00080020&date=%s", studyDateBuf);
    else if(strcmp(mode, "receive") == 0)
        sprintf_s(indexPath, "../index.htm?mode=receive&date=%s", receiveDateBuf);
    else
    {
        errorMessage = "mode´íÎó";
        goto remove_study_error;
    }
    cgiHeaderLocation(indexPath);
    cgiHeaderContentType("text/html");
    return 0;
}

size_t collectionToFileNameList(const char *xmlpath, list<Study> &studies, bool isPatient)
{
	MSXML2::IXMLDOMDocumentPtr pXmlDom;
	HRESULT hr = pXmlDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		index_errlog << "Failed to CreateInstance on an XML DOM." << endl;
		return -1;
	}
	if(!pXmlDom->load(xmlpath)) return -2;
	MSXML2::IXMLDOMNodeListPtr pStudyList;
	if(isPatient)
		pStudyList = pXmlDom->selectNodes("/wado_query/Patient/Study");
	else
		pStudyList = pXmlDom->selectNodes("/Collection/Study");
	if(!pStudyList) return -2;
	while(MSXML2::IXMLDOMNodePtr pNode = pStudyList->nextNode())
	{
		string studyUid;
		if(isPatient)
			studyUid = pNode->Getattributes()->getNamedItem("StudyInstanceUID")->text;
		else
			studyUid = pNode->text;
		if(studies.end() != find_if(studies.begin(), studies.end(), [&studyUid](Study &s) { return studyUid == s.uid; }))
			continue;
		MSXML2::IXMLDOMNodePtr studySizeAttr = pNode->Getattributes()->getNamedItem("StudyDescription");
		int studySize = 0;
		if(studySizeAttr) studySize = atoi(studySizeAttr->text);
		try
		{
			studies.push_back(Study(studyUid.c_str(), studySize));
		}
		catch(exception &e)
		{
			index_errlog << e.what() << endl;
		}
	}
	studies.sort([](const Study& s1, const Study &s2) { return s1.size > s2.size || (s1.size == s2.size && strcmp(s1.uid, s2.uid) > 0); } );
	return pStudyList->Getlength();
}
