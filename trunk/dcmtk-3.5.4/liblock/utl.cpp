#include <windows.h>
#include <iostream>
#include <regex>
using namespace std;

extern "C" unsigned int getLockNumber(const char *filter, const char *regxPattern, int isDirectory, char *filenamebuf)
{
	WIN32_FIND_DATA ffd;
	unsigned int lockNumber = 0;

	HANDLE hFind = FindFirstFile(filter, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) 
	{
		cerr << TEXT("FindFirstFile Error in ") << filter << endl;
		return -3;
	}
	
	regex pattern(TEXT(regxPattern));
	match_results<string::const_iterator> result;
	// List all the files in the directory with some info about them.
	do
	{
		if (isDirectory ? ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY : true)
		{
			string fileName(ffd.cFileName);
			if(regex_match(fileName, result, pattern))
			{
				char buffer[MAX_PATH];
				result[1].str().copy(buffer, result[1].length(), 0);
				buffer[result[1].length()] = '\0';
				sscanf_s(buffer, TEXT("%d"), &lockNumber);
				if(filenamebuf)
				{
					fileName.copy(filenamebuf, fileName.length(), 0);
					filenamebuf[fileName.length()] = '\0';
				}
				break;
			}
		}
	} while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
	return lockNumber;
}
