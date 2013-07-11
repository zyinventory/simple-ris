#include "stdafx.h"
#include <sstream>
#import <msxml3.dll>
using namespace std;

static ostringstream buffer;

void outputContent(bool error)
{
	string content = buffer.str();
	if(error)
		fprintf(cgiOut, "Content-type: text/plain; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
	else
		fprintf(cgiOut, "Content-type: text/xml; charset=GBK\r\nContent-Length: %d\r\n\r\n", content.length());
	fprintf(cgiOut, content.c_str());
}

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

	MSXML2::IXMLDOMProcessingInstructionPtr pi;
	pi = pXmlDom->createProcessingInstruction("xml", "version=\"1.0\" encoding=\"gbk\"");
	if (pi != NULL) pXmlDom->appendChild(pi);

	MSXML2::IXMLDOMElementPtr root = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "tdb_status", "");
	pXmlDom->appendChild(root);
	MSXML2::IXMLDOMElementPtr errorInfos = pXmlDom->createNode(MSXML2::NODE_ELEMENT, "error_infos", "");
	
	CSimpleIni::TNamesDepend sections;
	ini.GetAllSections(sections);
	CSimpleIni::TNamesDepend::iterator sec = sections.begin();
	while(sec != sections.end())
	{
		const char *currentSection = NULL, *currentKey = NULL, *currentValue = NULL;
		try
		{
			currentSection = (*sec).pItem;
			MSXML2::IXMLDOMElementPtr sectionNode = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentSection, "");

			CSimpleIni::TNamesDepend keys;
			ini.GetAllKeys((*sec).pItem, keys);
			CSimpleIni::TNamesDepend::iterator key = keys.begin();
			while(key != keys.end())
			{
				currentKey = (*key).pItem;
				MSXML2::IXMLDOMElementPtr item = pXmlDom->createNode(MSXML2::NODE_ELEMENT, currentKey, "");
				currentValue = ini.GetValue(currentSection, currentKey);
				item->appendChild(pXmlDom->createTextNode(currentValue));
				sectionNode->appendChild(item);
				currentKey = NULL;
				currentValue = NULL;
				++key;
			}
			root->appendChild(sectionNode);
		}
		catch(_com_error &ex) 
		{
			hasError = true;
			ostringstream errbuf;
			errbuf << "Éè±¸×´Ì¬´íÎó: 0x" << hex << ex.Error() << ',' << ex.ErrorMessage();
			if(currentSection)
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
	buffer << "<?xml version=\"1.0\" encoding=\"gbk\"?>" << root->xml;
	outputContent(false);
	return 0;
}