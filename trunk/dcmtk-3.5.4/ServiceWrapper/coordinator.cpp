// coordinator.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <comutil.h>
#import <mqoa.dll>
#import <msxml3.dll>

using namespace std;
using namespace MSMQ;
#include "commonlib.h"

static SECURITY_ATTRIBUTES logSA;
static size_t procnum;
static HANDLE *processHandles, *threadHandles, *logFileHandles;
static string *logFilePathArray;

void closeProcHandle(int i)
{
	CloseHandle(threadHandles[i]);
	threadHandles[i] = 0;
	CloseHandle(processHandles[i]);
	processHandles[i] = 0;
}

DWORD findIdle()
{
	size_t working = 0;
	DWORD  wait, result = 0;
	HANDLE *workingHandles = new HANDLE[procnum];
	//collect all working process
	for(size_t i = 0; i < procnum; ++i)
	{
		if(processHandles[i])
		{
			workingHandles[working] = processHandles[i];
			++working;
		}
	}

	if(working >= procnum)  // all process is working
	{
		wait = WaitForMultipleObjects(procnum, processHandles, FALSE, INFINITE);
		result = wait - WAIT_OBJECT_0;
		if(result >= 0 && result < procnum)
			closeProcHandle(result);
		else
			result = WAIT_FAILED;
	}
	else if(working > 0) // some process is working
	{
		wait = WaitForMultipleObjects(working, workingHandles, FALSE, 0);
		if(wait == WAIT_TIMEOUT) // all working process have not terminated, find empty process
		{
			for(result = 0; result < procnum; ++result)
			{
				if( ! processHandles[result] ) break;
			}
			if(result >= procnum) result = WAIT_FAILED;
		}
		else  // one process has terminated
		{
			int index = wait - WAIT_OBJECT_0;
			if(index >= 0 && index < static_cast<int>(working))
			{
				// find the process index of processHandles[], close it
				for(result = 0; result < procnum; ++result)
				{
					if(processHandles[result] == workingHandles[index])
					{
						closeProcHandle(result);
						break;
					}
				}
				if(result >= procnum) result = WAIT_FAILED;
			}
			else
				result = WAIT_FAILED;
		}
	}
	else // working == 0, no process
	{
		result = procnum;
	}
	delete[] workingHandles;
	return result;
}

void runCommand(string &command, int index)
{
	if(command.find("move") == 0) // start with "move"
	{
		string::size_type p;
		string src, dest;
		if(string::npos != (p = command.find(' ', 5)))
		{
			src = command.substr(5, p - 5);
			dest = command.substr(++p);
			if(GetFileAttributes(dest.c_str()) != INVALID_FILE_ATTRIBUTES)
			{
				if(remove(dest.c_str())) perror(dest.c_str());
			}
			else
				prepareFileDir(dest.c_str());
			if( rename(src.c_str(), dest.c_str()) ) perror(src.append(" -> ").append(dest).c_str());
		}
		else
		{
			cerr << TEXT("runCommand move命令格式错误:") << command << endl;
		}
	}
	else
	{
		PROCESS_INFORMATION procinfo;
		STARTUPINFO sinfo;

		memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
		memset(&sinfo, 0, sizeof(STARTUPINFO));
		sinfo.cb = sizeof(STARTUPINFO);

		if( ! logFileHandles[index] )
		{
			char timebuf[48];
			generateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_", timebuf, 48);
			ostringstream strbuf;
			strbuf << timebuf << index << "_coordinator.txt";
			logFilePathArray[index] = strbuf.str();
			prepareFileDir(logFilePathArray[index].c_str());
			HANDLE logFile = CreateFile(logFilePathArray[index].c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if(logFile == INVALID_HANDLE_VALUE)
			{
				_com_error ce(AtlHresultFromLastError());
				cerr << TEXT("runCommand未知错误：") << ce.ErrorMessage() << endl;
			}
			else
			{
				logFileHandles[index] = logFile;
			}
		}

		if(logFileHandles[index])
		{
			sinfo.dwFlags |= STARTF_USESTDHANDLES;
			sinfo.hStdOutput = logFileHandles[index];
			sinfo.hStdError = logFileHandles[index];
			sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		}

		char *commandLine = new char[command.length() + 1];
		copy(command.begin(), command.end(), stdext::checked_array_iterator<char*>(commandLine, command.length()));
		commandLine[command.length()] = '\0';
		if( CreateProcess(NULL, commandLine, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &sinfo, &procinfo) )
		{
			SetPriorityClass(procinfo.hProcess, PROCESS_MODE_BACKGROUND_BEGIN);
			processHandles[index] = procinfo.hProcess;
			threadHandles[index] = procinfo.hThread;
		}
		else
		{
			displayErrorToCerr("CreateProcess");
		}
		delete[] commandLine;
	}
}

void processMessage(IMSMQMessagePtr pMsg)
{
	HRESULT hr;
	if(pMsg->Label == _bstr_t(ARCHIVE_INSTANCE) || pMsg->Label == _bstr_t(ARCHIVE_STUDY))
	{
		try
		{
			MSXML2::IXMLDOMDocumentPtr pXml;
			hr = pXml.CreateInstance(__uuidof(MSXML2::DOMDocument30));
			if(VARIANT_FALSE == pXml->loadXML(pMsg->Body.bstrVal)) throw "load xml error";
			MSXML2::IXMLDOMNodePtr command = pXml->selectSingleNode("/wado_query/httpTag[@key='command']");
			if(command == NULL)
			{
				cerr << "no command: " << pXml->xml << endl;
				throw "no command";
			}
			string cmd(command->Getattributes()->getNamedItem("value")->text);
			DWORD index = findIdle();
			if(index != WAIT_FAILED)
			{
				if(index == procnum) index = 0;
				runCommand(cmd, index);
			}
			else
			{
				cerr << "find idle process failed:" << endl;
				cerr << cmd << endl;
			}
		}
		catch(_com_error &comErr)
		{
			cerr << TEXT("processMessage错误：") << comErr.ErrorMessage() << endl;
		}
		catch(...)
		{
			_com_error ce(AtlHresultFromLastError());
			cerr << TEXT("processMessage未知错误：") << ce.ErrorMessage() << endl;
		}
	}
	else
	{
		cerr << TEXT("processMessage unknown message: ") << pMsg->Label << TEXT(':');
		if(pMsg->Body.vt == VT_BSTR)
			cerr << pMsg->Body.pbstrVal;
		cerr << endl;
	}
}

void closeLogFile(int i)
{
	if(logFileHandles[i])
	{
		DWORD pos = SetFilePointer(logFileHandles[i], 0, NULL, FILE_CURRENT);
		CloseHandle(logFileHandles[i]);
		logFileHandles[i] = 0;
		if(pos == 0) DeleteFile(logFilePathArray[i].c_str());
#ifdef _DEBUG
		cout << "close log " << logFilePathArray[i] << endl;
#endif
	}
}

int pollQueue(const _TCHAR *queueName)
{
	HRESULT hr;
	try
	{
		IMSMQQueueInfoPtr pInfo;
		hr = pInfo.CreateInstance("MSMQ.MSMQQueueInfo");
		if(FAILED(hr)) throw _com_error(hr, NULL);
		pInfo->PathName = queueName;
		IMSMQQueuePtr pQueue = pInfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
		_variant_t waitStart(100); // 0.1 seconds
		_variant_t waitNext(1 * 1000); // 1 seconds
		while( ! GetSignalInterruptValue() )
		{
			IMSMQMessagePtr pMsg = pQueue->Receive(NULL, NULL, NULL, &waitStart);
			if(pMsg)
			{
				while(pMsg)
				{
					processMessage(pMsg);
					pMsg = pQueue->Receive(NULL, NULL, NULL, &waitNext);
				}
			}
			// no message, try cleaning
			DWORD index = findIdle();
			if(index == WAIT_FAILED)
				throw "findIdle error";
			else if(index == procnum) // no process, close all log file
				for(size_t i = 0; i < procnum; ++i) closeLogFile(i);
		}
		hr = pQueue->Close();
		if(FAILED(hr)) throw _com_error(hr, NULL);
		return 0;
	}
	catch(_com_error &comErr)
	{
		cerr << TEXT("pollQueue错误：") << comErr.ErrorMessage() << endl;
		return -10;
	}
	catch(const char *message)
	{
		cerr << TEXT("pollQueue错误：") << message << endl;
		return -10;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << TEXT("pollQueue unknown error: ") << ce.ErrorMessage() << endl;
		return -10;
	}
}

void waitAll()
{
	size_t working = 0;
	DWORD  result = 0;
	HANDLE *workingHandles = new HANDLE[procnum];
	//collect all working process
	for(size_t i = 0; i < procnum; ++i)
	{
		if(processHandles[i])
		{
			workingHandles[working] = processHandles[i];
			++working;
		}
	}
	// if some process is working, wait all
	if(working > 0)
		WaitForMultipleObjects(working, workingHandles, TRUE, INFINITE);
	delete[] workingHandles;
}

int commandDispatcher(const char *queueName, int processorNumber)
{
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);
	procnum = processorNumber;
	processHandles = new HANDLE[procnum];
	memset(processHandles, 0, sizeof(HANDLE) * procnum);
	threadHandles = new HANDLE[procnum];
	memset(threadHandles, 0, sizeof(HANDLE) * procnum);
	logFileHandles = new HANDLE[procnum];
	memset(logFileHandles, 0, sizeof(HANDLE) * procnum);
	logFilePathArray = new string[procnum];

	CoInitialize(NULL);
	int ret = pollQueue(queueName);
	CoUninitialize();

	waitAll();
	for(size_t i = 0; i < procnum; ++i)
	{
		if(processHandles[i]) closeProcHandle(i);
		closeLogFile(i);
	}
	delete processHandles;
	delete threadHandles;
	delete logFileHandles;
	delete[] logFilePathArray;
	return ret;
}
