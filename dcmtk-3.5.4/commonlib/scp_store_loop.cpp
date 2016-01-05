#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;
static int is_cr_lf(int c) { return (c == '\r' || c == '\n') ? 1 : 0; }
#define STRING_LTRIM(str) str.erase(str.begin(), find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(is_cr_lf))))

bool opt_verbose = false;
char pacs_base[MAX_PATH];
const char *sessionId;

static FILE_OLP_INST file_read;
static string cmd_buf;
static list<string> queued_cmd;
static size_t waitMilliSec = 0;
static int apc_func_state = APC_FUNC_NONE;
static HANDLE hDirNotify;

static void CALLBACK read_cmd_and_process(DWORD dwErr, DWORD cbRead, LPOVERLAPPED polp)
{
    LPFILE_OLP_INST foi = (LPFILE_OLP_INST)polp;
    if ((dwErr == 0) && cbRead)
    {
        polp->Offset += cbRead;
        cmd_buf.append(foi->chBuff, cbRead);
        string::iterator it;
        do
        {
            STRING_LTRIM(cmd_buf);
            it = find_if(cmd_buf.begin(), cmd_buf.end(), std::ptr_fun<int, int>(is_cr_lf));
            if(it != cmd_buf.end())
            {
                queued_cmd.push_back(cmd_buf.substr(0, it - cmd_buf.begin()));
                cmd_buf.erase(cmd_buf.begin(), it);
            }
        } while(it != cmd_buf.end());

        while(!queued_cmd.empty())
        {
            foi->fail = (process_cmd(queued_cmd.front()) == 0);
            queued_cmd.pop_front();
        }

        if(!foi->fail && !ReadFileEx(foi->hFileHandle, foi->chBuff, FILE_ASYN_BUF_SIZE, polp, read_cmd_and_process))
            dwErr = GetLastError();
    }
    
    if(foi->fail)
    {
        if(hDirNotify && hDirNotify != INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(hDirNotify);
            hDirNotify = NULL;
        }
        if(foi->hFileHandle && foi->hFileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(foi->hFileHandle);
            foi->hFileHandle = NULL;
        }
    }
    else
    {
        if(dwErr == ERROR_HANDLE_EOF)
        {
            FindNextChangeNotification(hDirNotify);
        }
        else if(dwErr != 0)
        {
            displayErrorToCerr("read_cmd_and_process()", dwErr);
            CloseHandle(foi->hFileHandle);
            foi->hFileHandle = NULL;
            foi->fail = true;
            if(hDirNotify && hDirNotify != INVALID_HANDLE_VALUE)
            {
                FindCloseChangeNotification(hDirNotify);
                hDirNotify = NULL;
            }
        }
    }
}

bool read_cmd_continous()
{
    if(!file_read.fail && !ReadFileEx(file_read.hFileHandle, file_read.chBuff, FILE_ASYN_BUF_SIZE, &file_read.oOverlap, read_cmd_and_process))
        displayErrorToCerr("read_cmd_continous()", GetLastError());
    return true;
}

COMMONLIB_API int scp_store_main_loop(const char *sessId, bool verbose)
{
    opt_verbose = verbose;
    sessionId = sessId;
    string fn("\\storedir\\");
    fn.append(sessionId);
    if(ChangeToPacsWebSub(pacs_base, MAX_PATH, fn.c_str()))
    {
        cerr << "无法切换工作目录" << endl;
        return -1;
    }
    if(_mkdir("log") && errno != EEXIST)
    {
        int en = errno;
        cerr << "mkdir log faile: " << strerror(en) << endl;
        return -1;
    }
    if(_mkdir("archdir") && errno != EEXIST)
    {
        int en = errno;
        cerr << "mkdir archdir faile: " << strerror(en) << endl;
        return -1;
    }
    
    memset(&file_read, 0, sizeof(FILE_OLP_INST));
    file_read.hFileHandle = CreateFile("cmove.txt", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);
    if(file_read.hFileHandle == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("cmove.txt", GetLastError());
        return -2;
    }

    hDirNotify = FindFirstChangeNotification(_getcwd(NULL, 0), FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hDirNotify == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("FindFirstChangeNotification()", GetLastError());
        return -2;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();
    cmd_buf.clear();
    cmd_buf.reserve(256);

/*
    if(!CreateNamedPipeToStaticHandle())
    {
        CloseHandle(tail);
        return -4;
    }
    if(!CreateClientProc(".")) // session level dicomdir
    {
        CloseHandle(tail);
        CloseNamedPipeHandle();
        return -5;
    }
*/
    size_t worker_num = 0, queue_size = 0;
    HANDLE *objs = NULL;
    WORKER_CALLBACK *cbs = NULL;
    objs = get_worker_handles(&worker_num, &queue_size, &cbs, hDirNotify);
    while(!file_read.fail || (worker_num + queue_size))
    {
        DWORD wr = WAIT_FAILED;
        if(objs) wr = WaitForMultipleObjectsEx(worker_num, objs, FALSE, waitMilliSec, TRUE);
        else
        {
            wr = SleepEx(waitMilliSec, TRUE);
            if(wr == 0) wr = WAIT_TIMEOUT;
        }
        // switch(wr)
        if(wr == WAIT_TIMEOUT)
        {
            
        }
        else if(wr == WAIT_IO_COMPLETION)
            ;
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + worker_num)
        {
            if(complete_worker(wr, objs, cbs, worker_num))
                apc_func_state |= APC_FUNC_Dicomdir;
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("WaitForMultipleObjectsEx() ", GetLastError());
            if(objs) delete[] objs;
            return -1;
        }

        if(objs) delete[] objs;
        if(cbs) delete[] cbs;
        objs = get_worker_handles(&worker_num, &queue_size, &cbs, hDirNotify);
    }
    if(objs) delete[] objs;
    if(cbs) delete[] cbs;
    if(file_read.hFileHandle && file_read.hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(file_read.hFileHandle);
        file_read.hFileHandle = NULL;
    }
    return 0;
}
