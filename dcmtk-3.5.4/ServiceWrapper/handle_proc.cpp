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

void handle_proc::close_proc()
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

handle_proc::~handle_proc()
{
    close_proc();
}

DWORD handle_proc::start_process(bool out_redirect)
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
        starting_process = true;
        if(opt_verbose) time_header_out(*pflog) << "handle_proc::start_process(" << exec_cmd << ") OK" << endl;
		return 0;
	}
	else
    {
        DWORD gle = GetLastError();
        if(logFile != INVALID_HANDLE_VALUE) CloseHandle(logFile);
        return displayErrorToCerr(__FUNCSIG__" CreateProcess() failed", gle, &*pflog);
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

handle_compress* handle_compress::make_handle_compress(const string &study_uid, const shared_ptr<relationship> &r, const shared_ptr<file_notify> &job, ostream &flog)
{
    char cmd[1024];
    if(strcmp("KEEP", job->get_expected_xfer().c_str()) == 0)
    {
        int mkdir_pos = sprintf_s(cmd, "cmd.exe /c move /y %s ", job->get_instance_filename().c_str());
        int ctn = mkdir_pos;
        ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s\\pacs\\archdir\\v0000000\\%s\\%s\\", GetPacsBase(), job->get_hash().c_str(), study_uid.c_str());
        strcpy_s(cmd + ctn, sizeof(cmd) - ctn, job->get_unique_filename().c_str());
        PrepareFileDir(cmd + mkdir_pos);
        string path(GetPacsTemp());
        path.append("\\pacs\\").append(job->get_path().c_str());
        return new handle_compress(job->get_unique_filename(), path, job->get_notify_filename(), cmd, "move", r, job, &flog);
    }
    else // compress
    {
        const char *verbose_flag = opt_verbose ? "-v" : "";
        const char *codec = ""; // JpegLess14SV1
        if(strcmp("Jp2kLossLess", job->get_expected_xfer().c_str()) == 0) codec = "--encode-jpeg2k-lossless";

#ifdef _DEBUG
        int mkdir_pos = 0;
        char cmd[1024] = __FILE__;
        char *p = strrchr(cmd, '\\');
        if(p)
        {
            ++p;
            mkdir_pos = p - cmd;
            mkdir_pos += sprintf_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmcjpeg.exe %s %s --uid-never %s ", verbose_flag, codec, job->get_instance_filename().c_str());
        }
        else
            mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s %s --uid-never %s ", GetPacsBase(), verbose_flag, codec, job->get_instance_filename().c_str());
#else
        char cmd[1024];
	    int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s %s --uid-never -ds %s ", GetPacsBase(), verbose_flag, codec, job->get_instance_filename().c_str());
#endif
        int ctn = mkdir_pos;
        ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s\\pacs\\archdir\\v0000000\\%s\\%s\\", GetPacsBase(), job->get_hash().c_str(), study_uid.c_str());
        strcpy_s(cmd + ctn, sizeof(cmd) - ctn, job->get_unique_filename().c_str());
        string path(GetPacsTemp());
        path.append("\\pacs\\").append(job->get_path().c_str());
        return new handle_compress(job->get_unique_filename(), path, job->get_notify_filename(), cmd, "dcmcjpeg", r, job, &flog);
    }
}

void handle_compress::print_state() const
{
    *pflog << "handle_compress::print_state() " << compr_job->get_unique_filename() << endl
        << "\tfile_notify.notify_filename: " << compr_job->get_notify_filename() << endl
        << "\tfile_notify.seq: " << dec << compr_job->get_seq() << endl
        << "\tfile_notify.studyUID: " << compr_job->get_study_uid() << endl;
    handle_proc::print_state();
}


void handle_compress::find_first_file_notify_to_start_process(HANDLE hSema, PROC_COMPR_LIST &proc_list, ostream &flog)
{
    char buff[MAX_PATH];
	DWORD dw = WAIT_OBJECT_0;
    shared_ptr<relationship> new_rela;
    shared_ptr<file_notify> new_file_notify;
    string current_notify_filename_base;
    do
    {
        new_rela = NULL;
        new_file_notify = NULL;
        RELA_POS_PAIR pr;
        if(proc_list.size() < WORKER_CORE_NUM) // some core idle
            pr = study_dir::find_first_job_in_studies(current_notify_filename_base);
        if(pr.first == NULL || pr.second == pr.first->get_file_queue_cend()) break; // no file_notify in queue
        new_rela = pr.first;
        new_file_notify = pr.second->second;
        if(new_file_notify == NULL) { pr.first->erase(pr.second->first); continue; } // wrong file_notify, erase it, next loop

        const string &unique_filename = new_file_notify->get_unique_filename();
        PROC_COMPR_LIST::iterator ite = find_if(proc_list.begin(), proc_list.end(),
            [&unique_filename](const handle_compress *p) { return (p->get_id().compare(unique_filename) == 0); });

        if(ite == proc_list.end()) // no same instance is in compressing
        {
            pr.first->erase(pr.second->first);

            string received_instance_file_path(GetPacsTemp());
            received_instance_file_path.append("\\pacs\\").append(new_file_notify->get_path()).append(1, '\\').append(new_file_notify->get_instance_filename());
            struct _stat64 fs;
            if(_stat64(received_instance_file_path.c_str(), &fs))
            {
                strerror_s(buff, errno);
                time_header_out(flog) << __FUNCSIG__" _stat64(" << received_instance_file_path << ") fail: " << buff << endl;
                fs.st_size = 0LL;
                continue;
            }

			dw = WaitForSingleObject(hSema, 0);
            if(dw == WAIT_OBJECT_0)
            {
                new_file_notify->set_rec_file_size(fs.st_size);
                handle_compress* compr_ptr = handle_compress::make_handle_compress(pr.first->get_study_uid(), new_rela, new_file_notify, flog);
                if(compr_ptr)
                {
					compr_ptr->set_priority(BELOW_NORMAL_PRIORITY_CLASS);
                    if(compr_ptr->start_process(false))
                    {   // failed
                        time_header_out(flog) << "watch_notify() handle_compress::start_process() failed: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << " " << new_file_notify->get_unique_filename() << endl;
                        delete compr_ptr;
                        ReleaseSemaphore(hSema, 1, NULL);
                    }
                    else// succeed
                    {
                        time_header_out(flog) << "watch_notify() handle_compress::start_process() OK: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << " " << new_file_notify->get_unique_filename() << endl;
                        proc_list.push_back(compr_ptr);
                    }
                }
                else
                {
                    time_header_out(flog) << "watch_notify() handle_compress::make_handle_compress() failed: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << endl;
                    ReleaseSemaphore(hSema, 1, NULL);
                }
            }
			else if(dw == WAIT_FAILED)
            {
                displayErrorToCerr("watch_notify() WaitForSingleObject(hSema)", GetLastError(), &flog);
                break; // break to main loop
            }
#ifdef _DEBUG
			else if(dw == WAIT_TIMEOUT)
            {
				time_header_out(flog) << "watch_notify() WaitForSingleObject(hSema) WAIT_TIMEOUT" << endl;
                break; // break to main loop
            }
#endif
        }
        else // the same instance is in compressing
        {
            time_header_out(flog) << "watch_notify() skip exist compress unique name "
                << new_file_notify->get_unique_filename() << " src notify: " << new_file_notify->get_notify_filename() << endl;
            current_notify_filename_base = new_file_notify->get_notify_filename();
        }
    } while(new_file_notify);
}
