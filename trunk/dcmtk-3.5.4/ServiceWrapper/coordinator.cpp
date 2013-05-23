// coordinator.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <sstream>
#include <list>
#include <iterator>
#include <algorithm>
#include <functional>
#include <comutil.h>
#import <mqoa.dll>
#import <msxml3.dll>

using namespace std;
using namespace MSMQ;
#include "commonlib.h"

typedef struct _WorkerProcess {
	string *instancePathCr, *csvPathCr, *studyUid; // command level
    HANDLE hProcess, hThread, mutexIdle, mutexRec, hChildStdInWrite; // process level
	HANDLE hLogFile; string *logFilePath; // slot level
} WorkerProcess, *PWorkerProcess, *LPWorkerProcess;

static string binPath;
static SECURITY_ATTRIBUTES logSA;
static size_t procnum;
static PWorkerProcess workers;
static list<WorkerProcess> dirmakers;

void closeProcHandle(WorkerProcess &wp)
{
	bool hasStudyUid = false;
	if(wp.csvPathCr) // dirmaker
	{
		cout << "close process csv = " << wp.csvPathCr;
		delete wp.csvPathCr;
		wp.csvPathCr = NULL;
	}
	else // worker
	{
		if(wp.studyUid)
		{
			cout << "close process study = " << *wp.studyUid;
			hasStudyUid = true;
		}
		if(wp.instancePathCr)
		{
			if(hasStudyUid)
				cout << ", instance = " << *wp.instancePathCr;
			else
				cout << "close process instance = " << *wp.instancePathCr;
		}
		else
			cout << endl;
	}

	if(wp.studyUid)
	{
		delete wp.studyUid;
		wp.studyUid = NULL;
	}
	if(wp.instancePathCr)
	{
		delete wp.instancePathCr;
		wp.instancePathCr = NULL;
	}
	if(wp.hChildStdInWrite)
	{
		CloseHandle(wp.hChildStdInWrite);
		wp.hChildStdInWrite = NULL;
	}
	if(wp.hThread)
	{
		CloseHandle(wp.hThread);
		wp.hThread = NULL;
	}
	if(wp.hProcess)
	{
		CloseHandle(wp.hProcess);
		wp.hProcess = NULL;
	}
	if(wp.mutexIdle)
	{
		CloseHandle(wp.mutexIdle);
		wp.mutexIdle = NULL;
	}
	if(wp.mutexRec)
	{
		CloseHandle(wp.mutexRec);
		wp.mutexRec = NULL;
	}
}

void closeProcHandle(int i)
{
	cout << "close process " << i << endl;
	closeProcHandle(workers[i]);
}

bool predicatorEqual(WorkerProcess wp, const char *studyUid)
{
	return wp.hProcess && wp.studyUid && 0 == wp.studyUid->compare(studyUid);
}

void runDcmmkdir(string &studyUid)
{
	list<WorkerProcess>::iterator iter = find_if(dirmakers.begin(), dirmakers.end(), bind2nd(ptr_fun(predicatorEqual), studyUid.c_str()));
	if(iter == dirmakers.end())
	{
		WorkerProcess wp;
		memset(&wp, 0, sizeof(WorkerProcess));
		unsigned int hashStudy = hashCode(studyUid.c_str());
		char archivedir[MAX_PATH];
		int archlen = sprintf_s(archivedir, MAX_PATH, "archdir\\%02X\\%02X\\%02X\\%02X\\%s",
			hashStudy >> 24 & 0xff, hashStudy >> 16 & 0xff, hashStudy >> 8 & 0xff, hashStudy & 0xff, studyUid.c_str());

		PROCESS_INFORMATION procinfo;
		STARTUPINFO sinfo;
		memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
		memset(&sinfo, 0, sizeof(STARTUPINFO));
		sinfo.cb = sizeof(STARTUPINFO);

		string command(binPath);
		command.append("dcmmkdir -ds +r --general-purpose-dvd -wu http://localhost/pacs/ +D #v\\#s\\DICOMDIR +id #v\\#s -qn ").append(studyUid).append(" @");
		string::size_type pos = 0;
		while(pos != string::npos)
		{
			pos = command.find( "#v\\#s", pos );
			if(pos != string::npos)
			{
				command.replace(pos, sizeof("#v\\#s") - 1, archivedir); // sizeof("#v\\#s") include NULL terminator, minus it
				pos += archlen;
			}
		}
		char *commandLine = new char[command.length() + 1];
		copy(command.begin(), command.end(), stdext::checked_array_iterator<char*>(commandLine, command.length()));
		commandLine[command.length()] = '\0';

		if( CreateProcess(NULL, commandLine, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &sinfo, &procinfo) )
		{
			cout << "create process: " << commandLine << endl;
			wp.hProcess = procinfo.hProcess;
			wp.hThread = procinfo.hThread;
			wp.studyUid = new string(studyUid);
			dirmakers.push_back(wp);
		}
		else
			displayErrorToCerr("create process error");
		delete[] commandLine;
	}
}

DWORD findIdleOrCompelete()
{
	size_t working = 0, idle;
	DWORD  wait, result = 0;
	HANDLE workingHandles[MAX_CORE];
	//collect all working process
	for(size_t i = 0; i < procnum; ++i)
	{
		if(workers[i].hProcess)
		{
			workingHandles[working] = workers[i].mutexIdle;
			++working;
		}
		else
			idle = i;
	}

	if(working >= procnum)  // all process is working
	{
		wait = WaitForMultipleObjects(procnum, workingHandles, FALSE, INFINITE);
		if(wait >= WAIT_ABANDONED_0 && wait < WAIT_ABANDONED_0 + procnum)
		{
			result = wait - WAIT_ABANDONED_0;
			closeProcHandle(result);
		}
		else if(wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + procnum)
			result = wait - WAIT_OBJECT_0;
		else
			result = WAIT_FAILED;
	}
	else if(working > 0) // some process is working
	{
		wait = WaitForMultipleObjects(working, workingHandles, FALSE, 0);
		if(wait == WAIT_TIMEOUT) // all working process have not terminated, find empty process
			return idle;
		else if(wait >= WAIT_ABANDONED_0 && wait < WAIT_ABANDONED_0 + working) // one process has terminated
		{
			int index = wait - WAIT_ABANDONED_0;
			// find the process index of workers, close it
			for(result = 0; result < procnum; ++result)
			{
				if(workers[result].mutexIdle == workingHandles[index])
				{
					closeProcHandle(result);
					break;
				}
			}
			if(result >= procnum) result = WAIT_FAILED;
		}
		else if(wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + working)
		{
			int index = wait - WAIT_OBJECT_0;
			// find the process index of workers, using it
			for(result = 0; result < procnum; ++result)
			{
				if(workers[result].mutexIdle == workingHandles[index]) break;
			}
			if(result >= procnum) result = WAIT_FAILED;
		}
		else
			result = WAIT_FAILED;
	}
	else // working == 0, no process
	{
		result = procnum;
	}

	if(result >= 0 && result < procnum && workers[result].studyUid)
	{
		string *studyUid = workers[result].studyUid, *instancePathCr = workers[result].instancePathCr;
		workers[result].studyUid = NULL;
		workers[result].instancePathCr = NULL;
		char labelBuffer[250], bodyBuffer[250];
		instancePathCr->insert(0, "archiving ");
		string instancePath = (*instancePathCr).substr(0, instancePathCr->length() - 1);
		RedirectMessageLabelEqualWith(labelBuffer, bodyBuffer, 250, instancePath.c_str(), studyUid->c_str());
		if(studyUid) delete studyUid;
		if(instancePathCr) delete instancePathCr;
	}
	return result;
}

void runArchiveInstance(string &cmd, const int index, string &studyUid)
{
	PROCESS_INFORMATION procinfo;
	STARTUPINFO sinfo;

	memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

	if( ! workers[index].hLogFile )
	{
		char timebuf[48];
		generateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_", timebuf, 48);
		ostringstream strbuf;
		strbuf << timebuf << index << "_coordinator.txt";
		workers[index].logFilePath = new string(strbuf.str());
		prepareFileDir(workers[index].logFilePath->c_str());
		workers[index].hLogFile = CreateFile(workers[index].logFilePath->c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(workers[index].hLogFile == INVALID_HANDLE_VALUE)
		{
			workers[index].hLogFile = NULL;
			delete workers[index].logFilePath;
			workers[index].logFilePath = NULL;
			_com_error ce(AtlHresultFromLastError());
			cerr << TEXT("runCommand未知错误：") << ce.ErrorMessage() << endl;
		}
	}

	if(workers[index].hLogFile)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdOutput = workers[index].hLogFile;
		sinfo.hStdError = workers[index].hLogFile;
	}

	string::size_type pos = cmd.rfind(' ');
	pos = cmd.rfind(' ', --pos);
	string iofile(cmd.substr(pos + 1));
	char *commandLine = new char[cmd.length() + 1];

	if( ! workers[index].hProcess )
	{
		cmd.erase(pos);
		char appendbuf[16];
		sprintf_s(appendbuf, 16, " -pn %d @ @", index);
		cmd.append(appendbuf);
		copy(cmd.begin(), cmd.end(), stdext::checked_array_iterator<char*>(commandLine, cmd.length()));
		commandLine[cmd.length()] = '\0';

		HANDLE hChildStdInRead;
		CreatePipe(&hChildStdInRead, &workers[index].hChildStdInWrite, &logSA, 0);
		SetHandleInformation(workers[index].hChildStdInWrite, HANDLE_FLAG_INHERIT, 0);
		sinfo.hStdInput = hChildStdInRead;

		if( CreateProcess(NULL, commandLine, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &sinfo, &procinfo) )
		{
			cout << "create process " << index << ':' << commandLine << endl;
			workers[index].hProcess = procinfo.hProcess;
			workers[index].hThread = procinfo.hThread;
			WaitForInputIdle(workers[index].hProcess, INFINITE);
			if(!workers[index].mutexIdle)
			{
				sprintf_s(commandLine, 20, "Global\\dcmcjpeg%d", index);
				string mutexIdleName(commandLine);
				while( !workers[index].mutexIdle && 
					!(workers[index].mutexIdle = OpenMutex(SYNCHRONIZE, FALSE, mutexIdleName.c_str())) && 
					GetLastError() == ERROR_FILE_NOT_FOUND)
				{
					cout << "waiting " << mutexIdleName << endl;
					Sleep(56);
				}
			}
			if(!workers[index].mutexRec)
			{
				sprintf_s(commandLine, 20, "Global\\receive%d", index);
				string mutexRecName(commandLine);
				while( workers[index].mutexIdle && !workers[index].mutexRec && 
					!(workers[index].mutexRec = OpenMutex(SYNCHRONIZE, FALSE, mutexRecName.c_str())) && 
					GetLastError() == ERROR_FILE_NOT_FOUND)
				{
					cout << "waiting " << mutexRecName << endl;
					Sleep(56);
				}
			}
			if(workers[index].mutexIdle && workers[index].mutexRec)
			{
				if(WAIT_OBJECT_0 != WaitForSingleObject(workers[index].mutexIdle, INFINITE))
				{
					cerr << "wait idle " << index << " failed" << endl;
					closeProcHandle(index);
				}
			}
		}
		else
		{
			displayErrorToCerr("CreateProcess");
			CloseHandle(workers[index].hChildStdInWrite);
			workers[index].hChildStdInWrite = NULL;
		}
		CloseHandle(hChildStdInRead);
	}

	if(workers[index].hProcess)
	{
		if(workers[index].mutexIdle && workers[index].mutexRec)
		{
			cout << "writing " << iofile << endl;
			iofile.append(1, '\n');  // for dcmcjpeg: cin.getline()
			DWORD bytesWritten;
			WriteFile(workers[index].hChildStdInWrite, iofile.c_str(), iofile.length(), &bytesWritten, NULL);
			if(WAIT_OBJECT_0 == SignalObjectAndWait(workers[index].mutexIdle, workers[index].mutexRec, INFINITE, FALSE))
			{
				workers[index].studyUid = new string(studyUid);
				string::size_type destPos = iofile.find(' ') + 1;
				string::size_type crPos = iofile.rfind('\n');
				if(string::npos == crPos)
				{
					string body(iofile.substr(destPos));
					string label("archiving ");
					label.append(body);
					SendCommonMessageToQueue(label.c_str(), body.c_str(), 7, studyUid.c_str());
					workers[index].instancePathCr = new string(body.append(1, '\n'));
				}
				else
				{
					string body(iofile.substr(destPos));
					workers[index].instancePathCr = new string(body);
					body.erase(body.rfind('\n'));
					string label("archiving ");
					label.append(body);
					SendCommonMessageToQueue(label.c_str(), body.c_str(), 7, studyUid.c_str());
				}
				ReleaseMutex(workers[index].mutexRec);
			}
			else
			{
				displayErrorToCerr("wait confirm");
				cerr << "get mutex failed, close process: " << commandLine << endl;
				closeProcHandle(index);
			}
		}
		else
		{
			cerr << "get mutex failed, close process: " << commandLine << endl;
			closeProcHandle(index);
		}
	}
	else
	{
		cerr << "create process " << index << " failed" << ':' << commandLine << endl;
	}
	delete[] commandLine;
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
			if(binPath.length() == 0)
			{
				string::size_type pos = cmd.find(' ');
				pos = cmd.rfind('\\', pos);
				binPath = cmd.substr(0, pos + 1);
				cout << "bin path is " << binPath << endl;
			}

			MSXML2::IXMLDOMNodePtr attrStudyUid = pXml->selectSingleNode("/wado_query/Patient/Study/@StudyInstanceUID");
			if(attrStudyUid == NULL)
			{
				cerr << "no study uid: " << pXml->xml << endl;
				throw "no study uid";
			}
			string studyUid(attrStudyUid->text);

			if(pMsg->Label == _bstr_t(ARCHIVE_INSTANCE))
			{
				string iofile;
				string::size_type pos = cmd.rfind(' ');
				pos = cmd.rfind(' ', --pos);
				iofile = cmd.substr(pos + 1);

				if(cmd.find("move") == 0) // start with "move"
				{
					char *buffer = new char[cmd.length() + 1];
					string::size_type pos, srcpos = cmd.find(' ') + 1;
					if(string::npos != (pos = cmd.find(' ', srcpos)))
					{
						copy(cmd.begin(), cmd.end(), stdext::checked_array_iterator<char*>(buffer, cmd.length() + 1));
						buffer[cmd.length()] = '\0';
						buffer[pos] = '\0';
						char *src = buffer + srcpos, *dest = buffer + pos + 1;
						if(GetFileAttributes(dest) != INVALID_FILE_ATTRIBUTES)
						{
							if(remove(dest)) perror(dest);
						}
						else
							prepareFileDir(dest);
						if( rename(src, dest) )
						{
							buffer[pos] = '>';
							perror(buffer);
						}
						//else
							//runDcmmkdir(studyUid);
					}
					else
					{
						cerr << TEXT("move命令格式错误:") << command << endl;
					}
					delete[] buffer;
				}
				else  // dcmcjpeg
				{
					DWORD index = findIdleOrCompelete();
					if(index != WAIT_FAILED)
					{
						if(index == procnum) index = 0;
						runArchiveInstance(cmd, index, studyUid);
					}
					else
					{
						cerr << "find idle process failed:" << endl;
						cerr << cmd << endl;
					}
				}
			}
			else  //pMsg->Label == _bstr_t(ARCHIVE_STUDY)
			{
				//runDcmmkdir(studyUid);
				string::size_type begin = cmd.find("-ic ");
				if(begin == string::npos)
				{
					begin = cmd.find("--input-csv ");
					if(begin != string::npos) begin += 12;
				}
				else
					begin += 4;
				if(begin != string::npos)
				{
					string::size_type end = cmd.find(' ', begin);
					SendCommonMessageToQueue("dcmmkdir", cmd.substr(begin, end - begin).c_str(), 0, studyUid.c_str());
				}
				else
					cerr << "process message error: no csv file: " << cmd << endl;
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
	if(workers[i].hLogFile)
	{
#ifdef _DEBUG
		cout << "closing log " << *workers[i].logFilePath << endl;
#endif
		DWORD pos = SetFilePointer(workers[i].hLogFile, 0, NULL, FILE_CURRENT);
		CloseHandle(workers[i].hLogFile);
		workers[i].hLogFile = NULL;
		if(pos == 0) DeleteFile(workers[i].logFilePath->c_str());
		delete workers[i].logFilePath;
		workers[i].logFilePath = NULL;
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
		_variant_t waitStart(1 * 1000); // 1 seconds
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
			DWORD index = findIdleOrCompelete();
			if(index == WAIT_FAILED)
				throw "findIdle error";
			else if(index == procnum) // no process, close all log file
			{
				for(size_t i = 0; i < procnum; ++i) closeLogFile(i);
			}
			else // close one process
			{
				closeProcHandle(index);
				closeLogFile(index);
			}
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
	HANDLE workingHandles[MAX_CORE];
	//collect all working process
	for(size_t i = 0; i < procnum; ++i)
	{
		if(workers[i].hProcess)
		{
			workingHandles[working] = workers[i].mutexIdle;
			++working;
		}
	}
	// if some process is working, wait all
	if(working > 0)
		WaitForMultipleObjects(working, workingHandles, TRUE, INFINITE);
}

int commandDispatcher(const char *queueName, int processorNumber)
{
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	procnum = processorNumber;
	workers = new WorkerProcess[procnum];
	memset(workers, 0, sizeof(WorkerProcess) * procnum);

	CoInitialize(NULL);
	int ret = pollQueue(queueName);
	CoUninitialize();

	waitAll();
	cout << "all process signaled" << endl;
	for(size_t i = 0; i < procnum; ++i)
	{
		closeProcHandle(i);
		closeLogFile(i);
	}
	delete workers;
	return ret;
}
