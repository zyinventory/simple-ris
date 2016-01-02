#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;

bool CMOVE_FILE_SECTION::StorePath(char *buf, size_t buf_size)
{
    if(buf_size < 63) return false;
    uidHash(studyUID, buf, buf_size);
    buf[8] = '\\';
    SeriesInstancePath(seriesUID, instanceUID, buf + 9, buf_size - 9);
    return true;
}

static list<CMOVE_LOG_CONTEXT> workers, index_queue;

static int create_worker_process(CMOVE_LOG_CONTEXT &lc)
{
    lc.hprocess = INVALID_HANDLE_VALUE;
    lc.hthread = INVALID_HANDLE_VALUE;
    char cmd[1024];
#ifdef _DEBUG
    int mkdir_pos = sprintf_s(cmd, "D:\\zy\\docs\\GitHub\\simple-ris\\dcmtk-3.5.4\\Debug\\dcmcjpeg.exe --encode-jpeg2k-lossless --uid-never %s ", lc.file.filename);
#else
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe --encode-jpeg2k-lossless --uid-never %s ", pacs_base, lc.file.filename);
#endif
    char *mkdir_ptr = cmd + mkdir_pos;
    int ctn = mkdir_pos;
    ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "archdir\\%s\\", lc.file.studyUID);
    lc.file.StorePath(cmd + ctn, sizeof(cmd) - ctn);
    if(!prepareFileDir(mkdir_ptr))
    {
        strerror(errno);
        return -1;
    }
    string logfile(lc.file.instanceUID);
    logfile.append(".txt");
    HANDLE log = CreateFile(logfile.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(log == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("create_worker_process() ", GetLastError());
        return -2;
    }
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdOutput, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdError, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if(0 == CreateProcess(NULL, cmd, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        displayErrorToCerr("create_worker_process() ", GetLastError());
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdError);
        return -3;
    }
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);
    lc.hprocess = pi.hProcess;
    lc.hthread = pi.hThread;
    lc.log = log;
    workers.push_back(lc);
    cerr << "trigger compress "  << lc.file.filename << endl;
    return 0;
}

static void close_log(const CMOVE_LOG_CONTEXT &lc)
{
    if(lc.log && lc.log != INVALID_HANDLE_VALUE)
	{
        ULARGE_INTEGER file_size;
        file_size.QuadPart = 0LL;
        file_size.LowPart = GetFileSize(lc.log, &file_size.HighPart);
        if(file_size.QuadPart == 0LL)
        {
            char filename[MAX_PATH];
            DWORD fnlen = GetFinalPathNameByHandle(lc.log, filename, MAX_PATH, FILE_NAME_NORMALIZED);
            if(fnlen > 0 || fnlen <= MAX_PATH)
            {
                CloseHandle(lc.log);
                if(! DeleteFile(filename)) displayErrorToCerr("close_log_context() ", GetLastError());
                else if(opt_verbose) cerr << "delete " << filename << " OK" << endl;
            }
        }
    }
}

static int find_complete_worker(CMOVE_LOG_CONTEXT &lc)
{
    size_t worker_num = workers.size();
    HANDLE *objs = new HANDLE[workers.size()];
    transform(workers.begin(), workers.end(), objs, [](const CMOVE_LOG_CONTEXT &clc) { return clc.hprocess; });
    DWORD wr = WaitForMultipleObjects(worker_num, objs, FALSE, 0);
    if(wr == WAIT_TIMEOUT)
    {
        delete[] objs;
        return worker_num;
    }
    else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + worker_num)
    {
        wr -= WAIT_OBJECT_0;
        list<CMOVE_LOG_CONTEXT>::iterator it = find_if(workers.begin(), workers.end(), 
            [objs, wr](CMOVE_LOG_CONTEXT &clc) { return clc.hprocess == objs[wr]; });
        delete[] objs;
        lc = *it;
        workers.erase(it);
        cerr << "trigger complete " << lc.file.filename << endl;
        return workers.size();
    }
    else
    {   // shall not reach here ...
        displayErrorToCerr("WaitForMultipleObjects() ", GetLastError());
        delete[] objs;
        return -1;
    }
}

bool run_index()
{
    if(index_queue.size() > 0)
    {
        CMOVE_LOG_CONTEXT lc = index_queue.front();
        index_queue.pop_front();
        // do index
        Sleep(0);
        close_log(lc);
        return true;
    }
    return false;
}

bool commit_file_to_workers(CMOVE_LOG_CONTEXT *plc)
{
    CMOVE_LOG_CONTEXT over_lc;
    int find_result;
    over_lc.hprocess = INVALID_HANDLE_VALUE;
    if(workers.size() > 0)
    {
wait_worker_again:
        find_result = find_complete_worker(over_lc);
        if(over_lc.hprocess != INVALID_HANDLE_VALUE)
        {
            CloseHandle(over_lc.hthread);
            over_lc.hthread = INVALID_HANDLE_VALUE;
            CloseHandle(over_lc.hprocess);
            over_lc.hprocess = INVALID_HANDLE_VALUE;
            index_queue.push_back(over_lc);
        }
        if(find_result >= worker_core_num)
        {
            /* if(opt_verbose) */cerr << "*"; // full of jobs
            if(!run_index()) Sleep(300);
            goto wait_worker_again;
        }
        else if(find_result < 0)
            return false;
    }
    bool compr_result = true;
    if(plc)compr_result = (0 == create_worker_process(*plc));
    else compr_result = (workers.size() > 0);

    if(workers.size() < worker_core_num)
    {
        /* if(opt_verbose) */cerr << "."; // any core idle
        run_index();
    }
    return compr_result;
}
