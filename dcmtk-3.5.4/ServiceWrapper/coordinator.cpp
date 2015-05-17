// coordinator.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#import <mqoa.dll>
#import <msxml3.dll>

using namespace std;
using namespace MSMQ;
#include "commonlib.h"

extern const char *dirmakerCommand;
extern bool opt_verbose;
IMSMQQueuePtr OpenOrCreateQueue(const char *queueName, MQACCESS access = MQ_SEND_ACCESS) throw(...);

static SECURITY_ATTRIBUTES logSA;
static size_t procnum;
static PWorkerProcess workers;
static list<WorkerProcess> dirmakers;

static void closeProcHandle(WorkerProcess &wp)
{
	if(opt_verbose)
	{
		time_header_out(cout) << "closeProcHandle: ";
		if(wp.hProcess)
			cout << "hProcess = " << wp.hProcess;
		else
			cout << "hProcess = NULL";
		cout << " start..." << endl;
	}
	if(wp.hChildStdInWrite)
	{
		CloseHandle(wp.hChildStdInWrite);
		wp.hChildStdInWrite = NULL;
	}
	// close handle
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
	BOOL termOK = TRUE;
	DWORD procExit = WAIT_OBJECT_0;
	if(wp.hProcess)
	{
		procExit = WaitForSingleObject(wp.hProcess, 1 * 1000);
		if(opt_verbose) time_header_out(cout) << "closeProcHandle: hProcess = " << wp.hProcess;
		if(procExit == WAIT_OBJECT_0)
		{
			if(opt_verbose) cout << ", close self ";
		}
		else if(procExit == WAIT_TIMEOUT)
		{
			if(opt_verbose) cout << ", close timeout, TerminateProcess(): ";
			termOK = TerminateProcess(wp.hProcess, -1);
		}
		else //if(procExit == WAIT_FAILED)
		{
			if(opt_verbose) cout << ", close failed, TerminateProcess(): ";
			termOK = TerminateProcess(wp.hProcess, -1);
		}

		if(opt_verbose)
		{
			if(termOK)
				cout << "OK";
			else
			{
				_com_error ce(AtlHresultFromLastError());
				cout << ce.ErrorMessage() << endl;
			}
		}
	}
	else
	{
		if(opt_verbose) time_header_out(cout) << "closeProcHandle: hProcess = NULL";
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
	if(wp.csvPath)
	{
		if(opt_verbose) cout << ", csv = " << *wp.csvPath;
		delete wp.csvPath;
		wp.csvPath = NULL;
	}
	else
	{
		if(opt_verbose) cout << ", csv = NULL";
	}
	/*
	bool hasStudyUid = false;
	if(wp.studyUid)
	{
		//cout << "close process study = " << *wp.studyUid;
		hasStudyUid = true;
	}
	if(wp.instancePath)
	{
		if(hasStudyUid)
			cout << ", instance = " << *wp.instancePath;
		else
			cout << "close process instance = " << *wp.instancePath;
	}
	if(wp.csvPath || wp.studyUid || wp.instancePath)
		cout << endl;
	*/
	if(wp.instancePath)
	{
		if(opt_verbose) cout << ", instance = " << *wp.instancePath;
		delete wp.instancePath;
		wp.instancePath = NULL;
	}
	else
	{
		if(opt_verbose) cout << ", instance = NULL";
	}

	if(wp.studyUid)
	{
		if(opt_verbose) cout << ", studyUID = " << *wp.studyUid;
		delete wp.studyUid;
		wp.studyUid = NULL;
	}
	else
	{
		if(opt_verbose) cout << ", studyUID = NULL";
	}
	if(opt_verbose) cout << endl;
}

static list<WorkerProcess>::iterator runDcmmkdir(string &studyUid)
{
	list<WorkerProcess>::iterator iter = find_if(dirmakers.begin(), dirmakers.end(), 
		[&studyUid](WorkerProcess &wp){return wp.hProcess && wp.studyUid && 0 == wp.studyUid->compare(studyUid);});
	if(iter != dirmakers.end()) return iter;

	WorkerProcess wp;
	memset(&wp, 0, sizeof(WorkerProcess));
	char hashBuf[9];
	__int64 hashStudy = uidHash(studyUid.c_str(), hashBuf, sizeof(hashBuf));
	char archivedir[MAX_PATH];
	int archlen = sprintf_s(archivedir, MAX_PATH, "archdir\\%c%c\\%c%c\\%c%c\\%c%c\\%s",
		hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], studyUid.c_str());

	PROCESS_INFORMATION procinfo;
	STARTUPINFO sinfo;
	memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

	char timebuf[48];
	generateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_", timebuf, 48);
	ostringstream strbuf;
	strbuf << timebuf << studyUid << ".txt";
	wp.logFilePath = new string(strbuf.str());
	prepareFileDir(wp.logFilePath->c_str());
	wp.hLogFile = CreateFile(wp.logFilePath->c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(wp.hLogFile == INVALID_HANDLE_VALUE)
	{
		wp.hLogFile = NULL;
		delete wp.logFilePath;
		wp.logFilePath = NULL;
		_com_error ce(AtlHresultFromLastError());
		time_header_out(cerr) << TEXT("run dcmmkdir error: ") << ce.ErrorMessage() << endl;
	}

	if(wp.hLogFile)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		sinfo.hStdOutput = wp.hLogFile;
		sinfo.hStdError = wp.hLogFile;
	}

	string command(dirmakerCommand);
	command.append(" -qn ").append(studyUid).append(" @");
	//command.append("dcmmkdir -ds +A +I +r --general-purpose-dvd -wu http://localhost/pacs/ +D #v\\#s\\DICOMDIR +id #v\\#s -qn ").append(studyUid).append(" @");
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
		if(opt_verbose) time_header_out(cout) << "create process: " << procinfo.hProcess << ", " << commandLine << endl
			<< "\tprocess log: " << *wp.logFilePath << endl;
		wp.hProcess = procinfo.hProcess;
		wp.hThread = procinfo.hThread;
		wp.studyUid = new string(studyUid);
		dirmakers.push_back(wp);
		iter = --(dirmakers.end());
	}
	else
		displayErrorToCerr(commandLine);

	if(wp.hLogFile)
		SetHandleInformation(wp.hLogFile, HANDLE_FLAG_INHERIT, 0);

	delete[] commandLine;
	return iter;
}

static void closeLogFile(WorkerProcess &wp)
{
	if(wp.hLogFile)
	{
		time_header_out(cout) << "closeLogFile: " << *wp.logFilePath << endl;
		DWORD pos = SetFilePointer(wp.hLogFile, 0, NULL, FILE_CURRENT);
		CloseHandle(wp.hLogFile);
		wp.hLogFile = NULL;
		if(pos == 0)
		{
			/*
			int waitCloseTime = 0;
			while(! DeleteFile(wp.logFilePath->c_str()) && waitCloseTime < 10 * 1000)
			{
				_com_error ce(AtlHresultFromLastError());
				time_header_out(cerr) << TEXT("closeLogFile error: ") << ce.ErrorMessage() << endl;
				waitCloseTime += 1000;
				Sleep(1000);
			}
			*/
			if(! DeleteFile(wp.logFilePath->c_str()))
			{
				_com_error ce(AtlHresultFromLastError());
				time_header_out(cerr) << TEXT("closeLogFile error: ") << ce.ErrorMessage() << endl;
			}
		}
		delete wp.logFilePath;
		wp.logFilePath = NULL;
	}
}

static void detectDcmmkdirProcessExit()
{
	size_t working = 0;
	DWORD  wait;

	//if(opt_verbose) time_header_out(cout) << "detectDcmmkdirProcessExit: ";
	if(dirmakers.size() == 0)
	{
		//if(opt_verbose) time_header_out(cout) << "no dirmaker" << endl;
		return;
	}
	/*
	else
	{
		if(opt_verbose) time_header_out(cout) << "start" << endl;
	}
	*/
	HANDLE *workingHandles = new HANDLE[dirmakers.size()];

	list<WorkerProcess>::iterator iter = dirmakers.begin();
	working = 0;
	while(iter != dirmakers.end())
	{
		workingHandles[working] = iter->hProcess;
		++iter;
		++working;
	}
	if(working > 0)
	{
		wait = WaitForMultipleObjects(working, workingHandles, FALSE, 0);
		if(wait >= WAIT_OBJECT_0 && wait < WAIT_OBJECT_0 + working)
		{
			wait -= WAIT_OBJECT_0;
			iter = find_if(dirmakers.begin(), dirmakers.end(), [workingHandles,wait](WorkerProcess &wp){return wp.hProcess == workingHandles[wait];});
			if(iter != dirmakers.end())
			{
				string *csvPath = NULL, *studyUid = iter->studyUid;
				iter->studyUid = NULL; // save string*, avoid closeProcHandle(*iter) release it.
				bool integrityStudy = true;
				if(iter->csvPath)
				{
					csvPath = iter->csvPath;
					iter->csvPath = NULL;  // save string*, avoid closeProcHandle(*iter) release it.
					integrityStudy = iter->integrityStudy;

					closeProcHandle(*iter);
					closeLogFile(*iter);
					dirmakers.erase(iter);

					if(opt_verbose) time_header_out(cout) << "detectDcmmkdirProcessExit: resume dirmaker " << *studyUid << endl;
					iter = runDcmmkdir(*studyUid);
					iter->csvPath = csvPath;
					csvPath = NULL;  // transfer string* to wp, don't release it manually.
					iter->integrityStudy = integrityStudy;
				}
				else
				{
					if(opt_verbose) time_header_out(cout) << "detectDcmmkdirProcessExit: try deleting queue: " << *studyUid << endl;
					bool restart = DeleteQueue(studyUid->c_str());
					if(opt_verbose)
					{
						if(restart)
							time_header_out(cout) << "detectDcmmkdirProcessExit: delete queue failed, resume dirmaker: " << *studyUid << endl;
						else
							time_header_out(cout) << "detectDcmmkdirProcessExit: delete queue OK: " << *studyUid << endl;
					}
					closeProcHandle(*iter);
					closeLogFile(*iter);
					dirmakers.erase(iter);
						
					if(restart) runDcmmkdir(*studyUid);
				}
				if(studyUid) delete studyUid;
			}
			else
				time_header_out(cerr) << "detectDcmmkdirProcessExit: dir maker " << wait << " not found" << endl;
		}
	}
	if(workingHandles) delete workingHandles;
	//if(opt_verbose) time_header_out(cout) << "detectDcmmkdirProcessExit: end" << endl;
}

/*
return:
	WAIT_FAILED : error
	procnum		: all idle
	other		: index of idle process
 */
static DWORD findIdleOrCompelete()
{
	size_t working = 0, idle = procnum;
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
			idle = idle == procnum ? i : idle; // find first idle workers[]'s index
	}

	if(working >= procnum)  // all process is working
	{
		wait = WaitForMultipleObjects(procnum, workingHandles, FALSE, INFINITE);
		if(wait >= WAIT_ABANDONED_0 && wait < WAIT_ABANDONED_0 + procnum)
		{
			result = wait - WAIT_ABANDONED_0;
			time_header_out(cerr) << "worker process " << result << " exit abandoned" << endl;
			closeProcHandle(workers[result]);
			closeLogFile(workers[result]);
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
					time_header_out(cerr) << "worker process " << result << " exit abandoned" << endl;
					closeProcHandle(workers[result]);
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

	detectDcmmkdirProcessExit();

	//if compress command is accomplished, clear command level variant in WorkerProcess.
	if(result >= 0 && result < procnum && workers[result].studyUid)
	{
		string *studyUid = workers[result].studyUid, *instancePath = workers[result].instancePath;
		workers[result].studyUid = NULL;
		workers[result].instancePath = NULL;
		if(instancePath)
		{
			string label(NOTIFY_COMPRESSED);
			label.append(*instancePath);
			//if(opt_verbose) time_header_out(cout) << "findIdleOrCompelete: message " << NOTIFY_COMPRESSED << ':' << instancePath->c_str() << " send" << endl;
			SendCommonMessageToQueue(label.c_str(), instancePath->c_str(), MQ_PRIORITY_COMPRESSED, studyUid->c_str());
			delete instancePath;
		}
		else
			time_header_out(cerr) << "findIdleOrCompelete error: worker process " << result << " has no instance path" << endl;
		
		if(studyUid)
		{
			//time_header_out(cout) << "compress OK, make sure dcmmkdir start" << endl;
			runDcmmkdir(*studyUid); // make sure dicomdir maker existing
			delete studyUid;
		}
		else
			time_header_out(cerr) << "findIdleOrCompelete error: worker process " << result << " has no study uid" << endl;
	}
	return result;
}

static void runArchiveInstance(string &cmd, const int index, string &studyUid)
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
		strbuf << timebuf << index << "_dcmcjpeg.txt";
		workers[index].logFilePath = new string(strbuf.str());
		prepareFileDir(workers[index].logFilePath->c_str());
		workers[index].hLogFile = CreateFile(workers[index].logFilePath->c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(workers[index].hLogFile == INVALID_HANDLE_VALUE)
		{
			workers[index].hLogFile = NULL;
			delete workers[index].logFilePath;
			workers[index].logFilePath = NULL;
			_com_error ce(AtlHresultFromLastError());
			time_header_out(cerr) << TEXT("run archive command error: ") << ce.ErrorMessage() << endl;
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
		CreatePipe(&hChildStdInRead, &workers[index].hChildStdInWrite, NULL, 0);
		SetHandleInformation(hChildStdInRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		sinfo.hStdInput = hChildStdInRead;

		if( CreateProcess(NULL, commandLine, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &sinfo, &procinfo) )
		{
			if(opt_verbose) time_header_out(cout) << "create process: " << procinfo.hProcess << " index " << index << ':' << commandLine << " > " << *workers[index].logFilePath << endl;
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
					//time_header_out(cout) << "waiting " << mutexIdleName << endl;
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
					//time_header_out(cout) << "waiting " << mutexRecName << endl;
					Sleep(56);
				}
			}
			if(workers[index].mutexIdle && workers[index].mutexRec)
			{
				if(WAIT_OBJECT_0 != WaitForSingleObject(workers[index].mutexIdle, INFINITE))
				{
					time_header_out(cerr) << "wait idle " << index << " failed" << endl;
					closeProcHandle(workers[index]);
				}
			}
		}
		else
		{
			displayErrorToCerr("CreateProcess");
			CloseHandle(workers[index].hChildStdInWrite);
			workers[index].hChildStdInWrite = NULL;
		}

		if(workers[index].hLogFile && !SetHandleInformation(workers[index].hLogFile, HANDLE_FLAG_INHERIT, 0))
		{
			_com_error ce(AtlHresultFromLastError());
			time_header_out(cerr) << TEXT("runArchiveInstance SetHandleInformation disable inherit: ") << ce.ErrorMessage() << endl;
		}
		CloseHandle(hChildStdInRead);
	}

	if(workers[index].hProcess)
	{
		if(workers[index].mutexIdle && workers[index].mutexRec)
		{
			//time_header_out(cout) << "writing " << iofile << endl;
			string iofileCr(iofile);
			iofileCr.append(1, '\n');  // for dcmcjpeg: cin.getline()
			DWORD bytesWritten;
			WriteFile(workers[index].hChildStdInWrite, iofileCr.c_str(), iofileCr.length(), &bytesWritten, NULL);
			if(opt_verbose) time_header_out(cout) << "write param to dcmcjpeg: " << iofile << endl;
			if(WAIT_OBJECT_0 == SignalObjectAndWait(workers[index].mutexIdle, workers[index].mutexRec, INFINITE, FALSE))
			{
				workers[index].studyUid = new string(studyUid);
				string::size_type destPos = iofile.find(' ') + 1;
				workers[index].instancePath = new string(iofile.substr(destPos));
				ReleaseMutex(workers[index].mutexRec);
			}
			else
			{
				displayErrorToCerr("wait confirm");
				time_header_out(cerr) << "get mutex failed, close process: " << commandLine << endl;
				closeProcHandle(workers[index]);
			}
		}
		else
		{
			time_header_out(cerr) << "get mutex failed, close process: " << commandLine << endl;
			closeProcHandle(workers[index]);
		}
	}
	else
	{
		time_header_out(cerr) << "create process " << index << " failed" << ':' << commandLine << endl;
	}
	delete[] commandLine;
}

static bool SendArchiveStudyCommand(WorkerProcess &wp)
{
	if(wp.csvPath == NULL) return false;

	string *csvPath = wp.csvPath;
	wp.csvPath = NULL;
	
	if(opt_verbose) time_header_out(cout) << "SendArchiveStudyCommand: ";
	if(wp.integrityStudy)
	{
		SendCommonMessageToQueue(NOTIFY_END_OF_STUDY, csvPath->c_str(), MQ_PRIORITY_DCMMKDIR, wp.studyUid->c_str());
		if(opt_verbose) cout << NOTIFY_END_OF_STUDY;
	}
	else
	{
		SendCommonMessageToQueue(NOTIFY_STUDY_NOT_INTEGRITY, csvPath->c_str(), MQ_PRIORITY_DCMMKDIR, wp.studyUid->c_str());
		if(opt_verbose) cout << NOTIFY_STUDY_NOT_INTEGRITY;
	}
	if(opt_verbose) cout << " : " << csvPath->c_str() << " : " << wp.studyUid->c_str() << endl;

	delete csvPath;
	wp.integrityStudy = false;
	return true;
}

/* flush csvPath, called repeatedly is safely.
 * find cached csvPath is not NULL.
 * if compress command is not exists, send archive study command with csvPath. */
static void checkStudyAccomplished()
{
	list<WorkerProcess>::iterator iter = dirmakers.begin();
	while((iter = find_if(iter, dirmakers.end(), [](WorkerProcess &wp){ return wp.csvPath != NULL; })) != dirmakers.end())
	{
		if(opt_verbose) time_header_out(cout) << "checkStudyAccomplished: dcmmkdir " << iter->hProcess << " cache found: " << iter->csvPath->c_str() << endl;
		size_t i;
		for(i = 0; i < procnum; ++i)
		{
			if(workers[i].hProcess && workers[i].studyUid && workers[i].studyUid->compare(*iter->studyUid) == 0)
				break; //some command has not accomplished
		}
		if(i >= procnum) //no more command running
			SendArchiveStudyCommand(*iter);
		else
		{
			if(opt_verbose) time_header_out(cout) << "checkStudyAccomplished: " << workers[i].instancePath->c_str() << " exists, skip SendArchiveStudyCommand(WorkerProcess &wp)" << endl;
		}
		++iter;
	}
}

static void processMessage(IMSMQMessagePtr pMsg)
{
	HRESULT hr;
	if(pMsg->Label == _bstr_t(ARCHIVE_INSTANCE) || pMsg->Label == _bstr_t(ARCHIVE_STUDY) || pMsg->Label == _bstr_t(ARCHIVE_STUDY_NOT_INTEGRITY))
	{
		try
		{
			MSXML2::IXMLDOMDocumentPtr pXml;
			hr = pXml.CreateInstance(__uuidof(MSXML2::DOMDocument30));
			if(VARIANT_FALSE == pXml->loadXML(pMsg->Body.bstrVal)) throw "load xml error";

			MSXML2::IXMLDOMNodePtr command = pXml->selectSingleNode("/wado_query/httpTag[@key='command']");
			if(command == NULL)
			{
				time_header_out(cerr) << "processMessage: no command: " << pXml->xml << endl;
				throw "no command";
			}
			string cmd(command->Getattributes()->getNamedItem("value")->text);

			MSXML2::IXMLDOMNodePtr attrStudyUid = pXml->selectSingleNode("/wado_query/Patient/Study/@StudyInstanceUID");
			if(attrStudyUid == NULL || attrStudyUid->text.length() < 5)
			{
				time_header_out(cerr) << "processMessage: bad study uid: " << pXml->xml << endl;
				throw "bad study uid";
			}
			string studyUid((LPCSTR)attrStudyUid->text);

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
						prepareFileDir(dest);
						int renresult = 0;
						if(::rename(src, dest)) renresult = errno;
						if(renresult)
						{
							if(renresult == EEXIST)
							{
								if(::remove(dest))
								{
									const char *msg = _strerror(NULL);
									cerr << "remove " << dest << " : " << msg << endl;
								}
								else
								{
									if(::rename(src, dest)) renresult = errno;
									else renresult = 0;
								}
							}
							else
							{
								int en = errno;
								const char *msg = _strerror(NULL);
								cerr << "unexcepted error " << en << ": " << msg << endl;
							}
						}

						if(renresult)
						{
							buffer[pos] = '>';
							cerr << buffer << " : " << strerror(renresult) << endl;
						}
						else
						{
							detectDcmmkdirProcessExit();
							string label(NOTIFY_COMPRESSED);
							label.append(dest);
							SendCommonMessageToQueue(label.c_str(), dest, MQ_PRIORITY_COMPRESSED, studyUid.c_str());
							runDcmmkdir(studyUid);
						}
					}
					else
					{
						time_header_out(cerr) << TEXT("processMessage: move command format error: ") << command << endl;
					}
					delete[] buffer;
				}
				else  // dcmcjpeg
				{
					if(opt_verbose) time_header_out(cout) << "processMessage: compress: findIdleOrCompelete..." << endl;
					DWORD index = findIdleOrCompelete();
					if(index != WAIT_FAILED)
					{
						if(index == procnum) index = 0;
						runArchiveInstance(cmd, index, studyUid);
						if(opt_verbose) time_header_out(cout) << "processMessage: compress: checkStudyAccomplished..." << endl;
						checkStudyAccomplished();  // suppose instance's transfer grouped by study
					}
					else
					{
						time_header_out(cerr) << "processMessage: compress: find idle process failed:" << endl;
						time_header_out(cerr) << cmd << endl;
					}
				}
			}
			else  //pMsg->Label == _bstr_t(ARCHIVE_STUDY) || pMsg->Label == _bstr_t(ARCHIVE_STUDY_NOT_INTEGRITY)
			{
				if(opt_verbose)
				{
					char uidBuffer[65];
					time_header_out(cout) << "processMessage: " << pMsg->Label << ':';
					if(extractStudyUid(uidBuffer, sizeof(uidBuffer), pMsg->Body.bstrVal)) cout << uidBuffer;
					cout << endl;
				}
				detectDcmmkdirProcessExit();

				string::size_type begin = cmd.find("-ic "), end;
				string csvPath;
				if(begin == string::npos)
				{
					begin = cmd.find("--input-csv ");
					if(begin != string::npos) begin += 12;
				}
				else
					begin += 4;
				if(begin != string::npos)
				{
					end = cmd.find(' ', begin);
					csvPath = cmd.substr(begin, end - begin);
				}
				else
					time_header_out(cerr) << "process message error: no csv file: " << cmd << endl;
				IMSMQQueuePtr pQueue = OpenOrCreateQueue(studyUid.c_str());
				pQueue->Close();
				//processMessage: receive message archive study, start dcmmkdir
				list<WorkerProcess>::iterator iter = runDcmmkdir(studyUid);
				//dcmmkdir shall poll the study-queue, get instance message, add the instance to dicomdir.
				//at the end of queue, generate dicomdir, generate index.
				if(iter != dirmakers.end() && csvPath.length() > 0)
				{
					// if old cached csvPath exists, then send dcmmkdir command with old cached csvPath and clear it.
					SendArchiveStudyCommand(*iter);
					// cache new csvPath
					if(iter->csvPath) delete iter->csvPath;
					iter->csvPath = new string(csvPath);
					iter->integrityStudy = (pMsg->Label == _bstr_t(ARCHIVE_STUDY));
					if(opt_verbose) time_header_out(cout) << "processMessage: csvPath " << csvPath << " cache to dirmaker " << iter->hProcess << endl;
				}
				else
					time_header_out(cerr) << "process message error: create or find dicomdir maker process failed" << endl;
				if(opt_verbose) time_header_out(cout) << "processMessage: end of archive: checkStudyAccomplished..." << endl;
				checkStudyAccomplished();
			}
		}
		catch(_com_error &comErr)
		{
			time_header_out(cerr) << TEXT("processMessage COM error: ") << comErr.ErrorMessage() << endl;
		}
		catch(...)
		{
			_com_error ce(AtlHresultFromLastError());
			time_header_out(cerr) << TEXT("processMessage unknown error: ") << ce.ErrorMessage() << endl;
		}
	}
	else
	{
		time_header_out(cerr) << TEXT("processMessage unknown message: ") << pMsg->Label << TEXT(':');
		if(pMsg->Body.vt == VT_BSTR)
			cerr << _bstr_t(pMsg->Body.bstrVal);
		cerr << endl;
	}
}

static int pollQueue(const _TCHAR *queueName)
{
	HRESULT hr;
	try
	{
		IMSMQQueuePtr pQueue = OpenOrCreateQueue(queueName, MQ_RECEIVE_ACCESS);
		_variant_t waitStart(1 * 1000); // 1 seconds
		_variant_t waitNext(1 * 1000); // 1 seconds
		bool alwaysIdle = true;
		while( ! GetSignalInterruptValue() )
		{
			IMSMQMessagePtr pMsg = pQueue->Receive(NULL, NULL, NULL, &waitStart);
			while(pMsg)
			{
				if(!alwaysIdle)
				{
					alwaysIdle = true;
					time_header_out(cout) << "pollQueue: receive message, start working" << endl;
					time_header_out(cout) << "VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV" << endl;
				}
				processMessage(pMsg);
				pMsg = pQueue->Receive(NULL, NULL, NULL, &waitNext);
			}
			if(alwaysIdle && opt_verbose)
			{
				time_header_out(cout) << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << endl;
				time_header_out(cout) << "pollQueue: no message, try cleaning: findIdleOrCompelete() and checkStudyAccomplished()" << endl;
				alwaysIdle = false;
			}
			// if completed compress found, send NOTIFY_COMPRESSED message, clear wp.instancePath and wp.studyUid.
			DWORD index = findIdleOrCompelete();

			checkStudyAccomplished();
			if(index == WAIT_FAILED)
				throw "findIdle error";
			else if(index == procnum) // no process, clean disk
			{
				//for(size_t i = 0; i < procnum; ++i) closeLogFile(workers[i]);
				autoCleanPacsDiskByStudyDate();
			}
			else // close one process
			{
				closeProcHandle(workers[index]);
				closeLogFile(workers[index]);
			}
		}
		hr = pQueue->Close();
		if(FAILED(hr)) throw _com_error(hr, NULL);
		return 0;
	}
	catch(_com_error &comErr)
	{
		time_header_out(cerr) << TEXT("pollQueue COM error: ") << comErr.ErrorMessage() << endl;
		return -10;
	}
	catch(const char *message)
	{
		time_header_out(cerr) << TEXT("pollQueue error: ") << message << endl;
		return -10;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		time_header_out(cerr) << TEXT("pollQueue unknown error: ") << ce.ErrorMessage() << endl;
		return -10;
	}
}

static void waitAll()
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

	int ret = pollQueue(queueName);

	waitAll();
	if(opt_verbose) time_header_out(cout) << "commandDispatcher: all process signaled" << endl;
	for(size_t i = 0; i < procnum; ++i)
	{
		closeProcHandle(workers[i]);
		closeLogFile(workers[i]);
	}
	delete workers;
	return ret;
}
