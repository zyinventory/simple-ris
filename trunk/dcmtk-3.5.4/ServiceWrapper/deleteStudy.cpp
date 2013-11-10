#include "stdafx.h"
#include <direct.h>
#include <commonlib.h>
#import <msxml3.dll>

using namespace std;

bool deleteTree(const char *dirpath)
{
	bool allOK = true;
	WIN32_FIND_DATA wfd;
	char fileFilter[MAX_PATH];
	strcpy_s(fileFilter, dirpath);
	PathAppend(fileFilter, "*.*");
	HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
	if (hDiskSearch == INVALID_HANDLE_VALUE)  // 如果没有找到或查找失败
	{
		DWORD winerr = GetLastError();
		if(ERROR_FILE_NOT_FOUND == winerr)
			cerr << fileFilter << " not found, skip" << endl;
		return false;
	}
	do
	{
		if (strcmp(wfd.cFileName, ".") == 0 || strcmp(wfd.cFileName, "..") == 0) 
			continue; // skip . ..
		fileFilter[strlen(dirpath)] = '\0';
		PathAppend(fileFilter, wfd.cFileName);
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(deleteTree(fileFilter))
			{
				if(_rmdir(fileFilter))
				{
					cerr << "rmdir " << fileFilter << " failed" << endl;
					allOK = false;
				}
			}
			else
			{
				allOK = false;
			}
		}
		else
		{
			if(remove(fileFilter))
			{
				cerr << "delete " << fileFilter << " failed" << endl;
				allOK = false;
			}
		}	
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // 关闭查找句柄
	return allOK;
}

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
		if(deleteTree(studyPath))
		{
			cerr << studyPath << " delete OK" << endl;
			if(_rmdir(studyPath))
				cerr << "rmdir study " << studyPath << " failed" << endl;
		}
		else
		{
			cerr << studyPath << " delete failed" << endl;
			allOK = false;
		}
	}
	return allOK;
}
