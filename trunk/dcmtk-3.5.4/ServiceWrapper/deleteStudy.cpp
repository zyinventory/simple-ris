#include "stdafx.h"
#include <commonlib.h>
#import <msxml3.dll>

using namespace std;

bool deleteDayStudy(const char *dayxml)
{
	MSXML2::IXMLDOMDocumentPtr dayIndex;
	dayIndex.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if(VARIANT_FALSE == dayIndex->load(dayxml)) return false;
	MSXML2::IXMLDOMNodeListPtr listptr = dayIndex->selectNodes("/Collection/Study");
	bool allOK = true;
	char studyPath[MAX_PATH];
	while(MSXML2::IXMLDOMNodePtr newStudy = listptr->nextNode())
	{
		_bstr_t studyUid = newStudy->Gettext();
		int hashStudy = hashCodeW((LPCWSTR)studyUid);
		sprintf_s(studyPath, MAX_PATH, "archdir\\%02X\\%02X\\%02X\\%02X\\%s",
			hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, (LPCSTR)studyUid);
		cerr << "deleting study " << studyPath << " ..." << endl;
		if(deleteTree(studyPath, &cerr))
		{
			cerr << studyPath << " delete OK" << endl;
		}
		else
		{
			cerr << studyPath << " delete failed" << endl;
			allOK = false;
		}
	}
	return allOK;
}
