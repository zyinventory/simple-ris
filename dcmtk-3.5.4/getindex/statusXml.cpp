#include "stdafx.h"
#include <lock.h>
#include <liblock.h>
#include <libb24.h>
#import <msxml3.dll>
using namespace std;

extern ostringstream buffer;
void outputContent(bool error);

int statusXml(CSimpleIni &ini, const char *statusFlag)
{
	bool hasError = false;
	MSXML2::IXMLDOMDocumentPtr pXmlDom;
	HRESULT hr = pXmlDom.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if (FAILED(hr))
	{
		buffer << "Failed to CreateInstance on an XML DOM." << endl;
		outputContent(true);
		return -1;
	}
	pXmlDom->preserveWhiteSpace = VARIANT_FALSE;
	pXmlDom->async = VARIANT_FALSE;

	MSXML2::IXMLDOMProcessingInstructionPtr pi = pXmlDom->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"gbk\"");
	if (pi != NULL) pXmlDom->appendChild(pi);

	MSXML2::IXMLDOMProcessingInstructionPtr pXslt = NULL;
	if(strcmp(statusFlag, "xml"))
	{
		pXslt = pXmlDom->createProcessingInstruction("xml-stylesheet", "type=\"text/xml\" href=\"../xslt/status.xsl\"");
		if (pXslt != NULL) pXmlDom->appendChild(pXslt);
	}

	MSXML2::IXMLDOMElementPtr root = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "tdb_status", "");
	pXmlDom->appendChild(root);
	MSXML2::IXMLDOMElementPtr errorInfos = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_infos", "");

	CSimpleIni::TNamesDepend sections;
	ini.GetAllSections(sections);
	CSimpleIni::TNamesDepend::iterator sec = sections.begin();
	while(sec != sections.end())
	{
		string currentSection;
		const char *currentKey = NULL, *currentValue = NULL;
		try
		{
			currentSection = (*sec).pItem;
			if(string::npos != currentSection.find("PUBLISHER", 0) || currentSection == "TDB_INFO"
				|| currentSection == "ACTIVE_JOB" || currentSection == "COMPLETE_JOB")
			{
				MSXML2::IXMLDOMElementPtr sectionNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentSection.c_str(), "");
				CSimpleIni::TNamesDepend keys;
				ini.GetAllKeys((*sec).pItem, keys);
				CSimpleIni::TNamesDepend::iterator key = keys.begin();
				while(key != keys.end())
				{
					currentKey = (*key).pItem;
					currentValue = ini.GetValue(currentSection.c_str(), currentKey);
					MSXML2::IXMLDOMElementPtr item;
					if(currentSection == "ACTIVE_JOB" || currentSection == "COMPLETE_JOB")
					{
						item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "JOB", "");
						item->setAttribute("id", currentValue);
					}
					else
					{
						item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentKey, "");
						item->appendChild(pXmlDom->createTextNode(currentValue));
					}
					sectionNode->appendChild(item);
					currentKey = NULL;
					currentValue = NULL;
					++key;
				}
				root->appendChild(sectionNode);
			}
			else  // treats it as job
			{
				MSXML2::IXMLDOMElementPtr sectionNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "JOB_STATUS", "");
				sectionNode->setAttribute("id", currentSection.c_str());
				CSimpleIni::TNamesDepend keys;
				ini.GetAllKeys((*sec).pItem, keys);
				CSimpleIni::TNamesDepend::iterator key = keys.begin();
				while(key != keys.end())
				{
					currentKey = (*key).pItem;
					currentValue = ini.GetValue(currentSection.c_str(), currentKey);
					MSXML2::IXMLDOMElementPtr item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentKey, "");
					item->appendChild(pXmlDom->createTextNode(currentValue));
					sectionNode->appendChild(item);
					currentKey = NULL;
					currentValue = NULL;
					++key;
				}
				root->appendChild(sectionNode);
			}
		}
		catch(_com_error &ex) 
		{
			hasError = true;
			ostringstream errbuf;
			errbuf << "Éè±¸×´Ì¬´íÎó: 0x" << hex << ex.Error() << ',' << ex.ErrorMessage();
			if(! currentSection.empty())
			{
				errbuf << ", [" << currentSection  << ']';
				if(currentKey) errbuf << ", " << currentKey	<< " = " << (currentValue ? currentValue : "");
			}
			MSXML2::IXMLDOMElementPtr errorInfo = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_info", "");
			errorInfo->appendChild(pXmlDom->createTextNode(errbuf.str().c_str()));
			errorInfos->appendChild(errorInfo);
		}
		++sec;
	}
	if(hasError) root->appendChild(errorInfos);
	buffer << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << (pXslt ? pXslt->xml : "") << root->xml;
	outputContent(false);
	return 0;
}

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
		buffer << "Failed to CreateInstance on an XML DOM." << endl;
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
	int lockNumber = getLockNumber(filename, FALSE, filename + 7);
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
			buffer << "ÐòÁÐºÅ´íÎó" << endl;
			outputContent(true);
			return -1;
		}
#ifdef NDEBUG
		Sleep(4000);
#endif
		DWORD serial = 0;
		if(!SetLock(8, &serial, 0, "fqE8km*O", "Tw2d@uJp", 0, 0))
		{
			buffer << "»ñÈ¡¼ÓÃÜËøÐòºÅ´íÎó:" << hex << LYFGetLastErr() << endl;
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
			buffer << "¼ÓÃÜËøÐ£Ñé´íÎó" << endl;
			outputContent(true);
			return -2;
		}
		
		// check batch mode flag
		bool isBatchMode = true;
		if(0 == loadPublicKeyContent2Pwd(filename, &siv, lockNumber, NULL, passwd))
		{
			DWORD data = 0;
			if(ReadLock(MODE_FLAG_POS, &data, passwd, 0, 0))
			{
				 isBatchMode = (1 == (data & 1));
			}
		}
		if(isBatchMode)
		{
			buffer << "ÊýÁ¿»òÐòÁÐºÅ´íÎó: " << chargekey << endl;
			outputContent(true);
			return -16;
		}

		int retCode = decodeCharge(chargekey, salt, Lock32_Function_Wrapper);
		switch(retCode)
		{
		case -1:
			buffer << "´íÎó1:" << chargekey << endl;
			outputContent(true);
			return -3;
		case -2:
			buffer << "´íÎó2:" << chargekey << endl;
			outputContent(true);
			return -4;
		case -3:
			buffer << "´íÎó3:" << chargekey << endl;
			outputContent(true);
			return -5;
		}
		DWORD box = ((unsigned int)retCode) >> 24;
		DWORD fileno = retCode & 0xFFFF;
		if(box > MAX_BOX || fileno != seq || fileno >= TOTAL_BUY)
		{
			buffer << "ÊýÁ¿»òÐòÁÐºÅ´íÎó:" << chargekey << endl;
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
					buffer << "ÊÚÈ¨´íÎó" << endl;
					outputContent(true);
					return -1;
				}
			}
			else
			{
				buffer << "ÊÚÈ¨´íÎó" << endl;
				outputContent(true);
				return -1;
			}
		}
		else
		{
			buffer << "ÊÚÈ¨´íÎó" << endl;
			outputContent(true);
			return -1;
		}

		char timeBuffer[16];
		generateTime(DATE_FORMAT_COMPACT, timeBuffer, sizeof(timeBuffer));
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
					if(licenseCount >= 0 && (licenseCount + increase) <= 0xFFFF)
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
						errorMessage = "³äÖµÊýÁ¿²»ÄÜ³¬¹ý65535";
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
			buffer << "³äÖµÈÕÖ¾Ð´Èë´íÎó£¬ÇëÖØÊÔ" << endl;
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
					buffer << "ÊÚÈ¨´íÎó" << endl;
					outputContent(true);
					return -1;
				}
			}
			else
			{
				buffer << "ÊÚÈ¨´íÎó" << endl;
				outputContent(true);
				return -1;
			}
		}
		else
		{
			buffer << "ÊÚÈ¨´íÎó" << endl;
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
	buffer << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << (pXslt ? pXslt->xml : "") << root->xml;
	outputContent(false);
	return 0;
}

int removeStudy(const char *flag)
{
	ostringstream errorMessageStream;
	char studyUID[65], patientID[65], indexPath[MAX_PATH];
	if(cgiFormNotFound != cgiFormString("studyUID", studyUID, 65) && strlen(studyUID) > 0)
	{
		int hashStudy = hashCode(studyUID);
		sprintf_s(indexPath, MAX_PATH, "archdir\\%02X\\%02X\\%02X\\%02X\\%s",
			hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
		if(deleteTree(indexPath, &errorMessageStream))
		{
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%02X\\%02X\\%02X\\%02X\\%s.xml",
				hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
			int rmXml = remove(indexPath);
			if(rmXml && errno == ENOENT) rmXml = 0;
			sprintf_s(indexPath, MAX_PATH, "indexdir\\0020000d\\%02X\\%02X\\%02X\\%02X\\%s.txt",
				hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUID);
			int rmTxt = remove(indexPath);
			if(rmTxt && errno == ENOENT) rmTxt = 0;
			if(rmXml || rmTxt)
				errorMessageStream << "¼ì²éÍ¼ÏñÉ¾³ý³É¹¦, ¼ì²éË÷ÒýÉ¾³ýÊ§°Ü" << endl;
			else
				errorMessageStream << "¼ì²éÉ¾³ý³É¹¦" << endl;
		}
		else
			errorMessageStream << "¼ì²éÉ¾³ýÊ§°Ü" << endl;
	}
	else
		errorMessageStream << "¼ì²éUID´íÎó" << endl;

	if(cgiFormNotFound != cgiFormString("patientID", patientID, 65) && strlen(patientID) > 0)
	{
		if(!deleteStudyFromPatientIndex(patientID, studyUID))
			errorMessageStream << "»¼ÕßË÷ÒýÉ¾³ý´íÎó" << endl;
	}
	else
		errorMessageStream << "»¼ÕßID´íÎó" << endl;

	string errorMessage = errorMessageStream.str();
	errorMessageStream.clear();
	fprintf(cgiOut, "Content-Type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", errorMessage.size());
	fprintf(cgiOut, errorMessage.c_str());
	return 0;
}
