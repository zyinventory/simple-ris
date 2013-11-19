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
	char countBuffer[12] = "", lock_passwd[9] = "", filename[64] = "..\\etc\\*.key";
	DWORD lockNumber = getLockNumber(filename, "^(\\d{8})\\.key$", FALSE, filename + 7);
	SEED_SIV siv;
	if(0 == loadPublicKeyContent(filename, &siv, lockNumber, lock_passwd))
	{
		if(!invalidLock("..\\etc\\license.key", filename, &siv))
		{
			licenseCount = currentCount(lock_passwd);
			if(licenseCount < 0 || licenseCount > 0xffff) licenseCount = 0;
		}
	}

	MSXML2::IXMLDOMElementPtr key;
	key = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "key", "");
	// charge?
	char chargekey[24] = "";
	if(flag && strlen(flag) && !strcmp("charge", flag)
		&& cgiFormNotFound != cgiFormString("password", chargekey, sizeof(chargekey)) && strlen(chargekey) > 0)
	{
		DWORD serial = 0;
		Sleep(4000);
		int retCode = SetLock(8, &serial, 0, NULL, lock_passwd, 0, 0);
		if(retCode)
		{
			buffer << "»ñÈ¡¼ÓÃÜËøÐòºÅ´íÎó:" << retCode << endl;
			outputContent(true);
			return -2;
		}

		retCode = decodeCharge(chargekey, serial, Lock32_Function_Wrapper);
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
		DWORD box = ((unsigned int)retCode) >> 10;
		DWORD fileno = retCode & 0x3FF;  // TOTAL_BUY + 32bits, 32bits is 16bits counter + 16bits reserve
		if(box > MAX_BOX || fileno > TOTAL_BUY)
		{
			buffer << "ÊýÁ¿´íÎó:" << chargekey << endl;
			outputContent(true);
			return -6;
		}

		char timeBuffer[16];
		generateTime(DATE_FORMAT_COMPACT, timeBuffer, sizeof(timeBuffer));
		ofstream chargeLog("pacs_log\\charge.log", ios_base::app, _SH_DENYRW);
		if(!chargeLog.fail())
		{
			chargeLog << chargekey << '\t' << timeBuffer << '\t';
			int sectionNumber = fileno / 64, offset = (fileno % 64) / 8;
			unsigned char bytemask = 0x80, section[8];
			bytemask >>= (fileno % 8);
			retCode = ReadLock(sectionNumber, section, lock_passwd, 0, 0);
			if(retCode == 0)
			{
				if((section[offset] & bytemask) == 0)
				{
					section[offset] |= bytemask;
					retCode = WriteLock(sectionNumber, section, lock_passwd, 0, 0);
					if(retCode == 0)
					{
						retCode = ReadLock(15, section, lock_passwd, 0, 0);
						if(retCode == 0)
						{
							increase = box * 50;
							int avoidOverflow = *(reinterpret_cast<WORD*>(section) + 3);
							avoidOverflow += increase;
							if(avoidOverflow >= 0 && avoidOverflow <= 0xFFFF)
							{
								*(reinterpret_cast<WORD*>(section) + 3) += increase;
								retCode = WriteLock(15, section, lock_passwd, 0, 0);
								if(retCode == 0)
								{
									chargeLog << "OK:" << increase << endl;
									oldCount = licenseCount;
									licenseCount = *(reinterpret_cast<WORD*>(section) + 3);
								}
								else
									errorMessage = "ÊýÁ¿Ð´Èë´íÎó";
							}
							else
								errorMessage = "³äÖµÊýÁ¿²»ÄÜ³¬¹ý65535";
						}
						else
							errorMessage = "ÊýÁ¿¶ÁÈ¡´íÎó";
					}
					else
						errorMessage = "´æ´¢Ð´Èë´íÎó";
				}
				else
					errorMessage = "´ËÃÜÂëÒÑ³ä¹ý";
			}
			else
				errorMessage = "´æ´¢¶ÁÈ¡´íÎó";
			if(errorMessage.length() > 0) chargeLog << "error:" << errorMessage << endl;
			chargeLog.close();
		}
		else
		{
			buffer << "·þÎñÆ÷Ã¦£¬ÇëÖØÊÔ" << endl;
			outputContent(true);
			return -9;
		}
		key->appendChild(pXmlDom->createTextNode(chargekey));
		root->appendChild(key);
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