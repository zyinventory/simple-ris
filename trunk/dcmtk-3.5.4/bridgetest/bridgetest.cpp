// bridgetest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <sys/timeb.h>
#include <io.h>

using namespace std;
#define FILENAME "count.txt"
int _tmain(int argc, _TCHAR* argv[])
{
  fstream fileStream;
  for(int i = 0; i < 10; i++)
  {
	::Sleep(100);
	if( _access_s(FILENAME, 0) == 0 )
	  fileStream.open(FILENAME, ios::out | ios::in | ios::ate, _SH_DENYRW);
	else
	  fileStream.open(FILENAME, ios::out | ios::in | ios::app, _SH_DENYRW);
	if(fileStream.bad()) cout << "bad" << endl;
	else if(fileStream.eof()) cout << "eof" << endl;
	else if(fileStream.fail()) cout << "fail" << endl;
	else break;
  }

  if( fileStream.good() )
  {
	char c;
	cin >> c;
	int number;
	fileStream.seekg(0, ios::beg);
	fileStream >> number;
	if( ! fileStream.good() )
	{
	  number = 0;
	  if(fileStream.bad()) cout << "bad" << endl;
	  else if(fileStream.eof()) cout << "eof" << endl;
	  else if(fileStream.fail()) cout << "fail" << endl;
	}
	++number;
	fileStream.seekp(0, ios::beg);
	fileStream.clear();
	fileStream << number << endl;
	fileStream.close();
  }
  else
  {
	struct _timeb now;
	errno_t err = _ftime_s(&now);
	LARGE_INTEGER time_ms;
	time_ms.QuadPart = now.time * 1000 + now.millitm;
	cout << time_ms.QuadPart << endl;
	cout << time_ms.LowPart << endl;
  }
}
