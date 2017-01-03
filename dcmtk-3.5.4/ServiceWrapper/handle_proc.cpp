#include "stdafx.h"

using namespace std;
using namespace handle_context;

handle_proc& handle_proc::operator=(const handle_proc &r)
{
    base_dir::operator=(r);
    hlog = r.hlog;
    exec_cmd = r.exec_cmd;
    exec_name = r.exec_name;
    log_path = r.log_path;
    procinfo = r.procinfo;
	priority = r.priority;
    return *this;
}

DWORD handle_proc::set_priority(DWORD p)
{
	switch(p)
	{
	case ABOVE_NORMAL_PRIORITY_CLASS:
		priority = ABOVE_NORMAL_PRIORITY_CLASS;
		break;
	case BELOW_NORMAL_PRIORITY_CLASS:
		priority = BELOW_NORMAL_PRIORITY_CLASS;
		break;
	case HIGH_PRIORITY_CLASS:
		priority = HIGH_PRIORITY_CLASS;
		break;
	case IDLE_PRIORITY_CLASS:
		priority = IDLE_PRIORITY_CLASS;
		break;
	case REALTIME_PRIORITY_CLASS:
		priority = REALTIME_PRIORITY_CLASS;
		break;
	case NORMAL_PRIORITY_CLASS:
		priority = NORMAL_PRIORITY_CLASS;
		break;
	default:
		break;
	}
	return priority;
}

void handle_proc::print_state() const
{
    *pflog << "handle_proc::print_state()" << endl
        << "\texec_name: " << exec_name << endl
        << "\tlog_path: " << log_path << endl
        << "\texec_cmd: " << exec_cmd << endl
		<< "\tpriority: ";
	switch(priority)
	{
	case ABOVE_NORMAL_PRIORITY_CLASS:
		*pflog << "ABOVE_NORMAL_PRIORITY_CLASS";
		break;
	case BELOW_NORMAL_PRIORITY_CLASS:
		*pflog << "BELOW_NORMAL_PRIORITY_CLASS";
		break;
	case HIGH_PRIORITY_CLASS:
		*pflog << "HIGH_PRIORITY_CLASS";
		break;
	case IDLE_PRIORITY_CLASS:
		*pflog << "IDLE_PRIORITY_CLASS";
		break;
	case REALTIME_PRIORITY_CLASS:
		*pflog << "REALTIME_PRIORITY_CLASS";
		break;
	case NORMAL_PRIORITY_CLASS:
		*pflog << "NORMAL_PRIORITY_CLASS";
		break;
	default:
		*pflog << priority;
		break;
	}
	*pflog << endl;
    base_dir::print_state();
}

handle_proc::~handle_proc()
{
    if(strcmp(exec_name.c_str(), "dcmcjpeg") || opt_verbose) time_header_out(*pflog) << "handle_proc destory " << exec_name << ": "
        << hex << setw(8) << setfill('0') << uppercase << procinfo.hThread << ", " << procinfo.hProcess << endl;
    if(procinfo.hThread && procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(procinfo.hThread);
    if(procinfo.hProcess && procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(procinfo.hProcess);
    if(hlog && hlog != INVALID_HANDLE_VALUE)
    {
        DWORD file_size = GetFileSize(hlog, NULL);
        CloseHandle(hlog);
        if(file_size == 0)
        {
            if(_unlink(log_path.c_str()))
            {
                char msg[1024];
                strerror_s(msg, errno);
                time_header_out(*pflog) << "handle_proc::~handle_proc() _unlink(" << log_path << ") failed: " << msg << endl;
            }
        }
    }
}

int handle_proc::start_process(bool out_redirect)
{
    STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

    if(procinfo.hThread) CloseHandle(procinfo.hThread);
    if(procinfo.hProcess) CloseHandle(procinfo.hProcess);
    memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
    if(hlog && hlog != INVALID_HANDLE_VALUE) CloseHandle(hlog);
    hlog = NULL;

	HANDLE logFile = INVALID_HANDLE_VALUE;
    char buff[FILE_BUF_SIZE];
    if(out_redirect)
    {
	    SECURITY_ATTRIBUTES logSA;
	    logSA.bInheritHandle = TRUE;
	    logSA.lpSecurityDescriptor = NULL;
	    logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

        in_process_sequence_dll(buff, sizeof(buff), "pacs_log\\");
        string logFilePath(buff);
        string::size_type pos = logFilePath.find('\\');
        pos +=5;
        logFilePath.insert(pos, 1, '\\');
        pos +=3;
        logFilePath.insert(pos, 1, '\\');
        pos +=3;
        logFilePath.insert(pos, 1, '\\').append(1, '_').append(exec_name).append(".txt");

        if(PrepareFileDir(logFilePath.c_str()))
            logFile = CreateFile(logFilePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	    if(logFile == INVALID_HANDLE_VALUE)
        {
            string msg(__FUNCSIG__" create log file failed: ");
            msg.append(logFilePath);
            displayErrorToCerr(msg.c_str(), GetLastError(), pflog);
        }
        else
	    {
		    sinfo.dwFlags |= STARTF_USESTDHANDLES;
		    sinfo.hStdOutput = logFile;
		    sinfo.hStdError = logFile;
		    sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            log_path = logFilePath;
	    }
    }

    strcpy_s(buff, FILE_BUF_SIZE, exec_cmd.c_str());
    BOOL inheritance = out_redirect ? TRUE : FALSE;
    if( CreateProcess(NULL, buff, NULL, NULL, inheritance, CREATE_NEW_PROCESS_GROUP | priority, NULL, get_path().c_str(), &sinfo, &procinfo) )
	{
        if(logFile != INVALID_HANDLE_VALUE)
        {
            if(FALSE == DuplicateHandle(GetCurrentProcess(), logFile, GetCurrentProcess(), &hlog, DUPLICATE_SAME_ACCESS, FALSE, 
                DUPLICATE_CLOSE_SOURCE)) // logFile will be closed whether DuplicateHandle() successfully or not.
                hlog = NULL;
        }
        if(opt_verbose) time_header_out(*pflog) << "handle_proc::start_process(" << exec_cmd << ") OK" << endl;
		return 0;
	}
	else
    {
        DWORD gle = GetLastError();
        if(logFile != INVALID_HANDLE_VALUE) CloseHandle(logFile);
        return displayErrorToCerr(__FUNCSIG__, gle, &*pflog);
    }
}

bool handle_proc::make_proc_ris_integration(const NOTIFY_FILE_CONTEXT *pnfc, const string &prog_path, ostream &flog)
{
    char path[MAX_PATH], hash[9];

    HashStr(pnfc->file.patientID, hash, sizeof(hash));
    sprintf_s(path, "%s\\pacs\\indexdir\\00100020\\%c%c\\%c%c\\%c%c\\%c%c", GetPacsBase(),
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
    if(!MkdirRecursive(path))
    {
        time_header_out(flog) << "handle_ris_integration::make_handle_ris_integration() can't create dir " << path << endl;
        return NULL;
    }

    HashStr(pnfc->file.studyUID, hash, sizeof(hash));
    sprintf_s(path, "%s\\pacs\\indexdir\\0020000d\\%c%c\\%c%c\\%c%c\\%c%c", GetPacsBase(),
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
    if(!MkdirRecursive(path))
    {
        time_header_out(flog) << "handle_ris_integration::make_handle_ris_integration() can't create dir " << path << endl;
        return NULL;
    }

    string cmd(GetPacsBase());
    cmd.append(1, '\\').append(prog_path).append(1, ' ').append(pnfc->file.patientID)
        .append(1, ' ').append(pnfc->file.studyUID).append(1, ' ').append(pnfc->study.accessionNumber);
    handle_proc *pris = new handle_proc(pnfc->file.studyUID, path, "", cmd, "RisIntegration", &flog);
    if(pris)
    {
        DWORD gle = pris->start_process(false);
        if(gle)
        {
            displayErrorToCerr(__FUNCSIG__" start_process()", gle, &flog);
            delete pris;
            return false;
        }
        else
        {
            delete pris;
            return true;
        }
    }
    else return false;
}