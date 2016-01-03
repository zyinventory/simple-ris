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
static int last_run_apc_func = APC_FUNC_ALL;

static void CALLBACK CompletedReadRoutine(DWORD dwErr, DWORD cbRead, LPOVERLAPPED polp)
{
    LPFILE_OLP_INST foi = (LPFILE_OLP_INST)polp;
    if ((dwErr == 0) && polp->InternalHigh)
    {
        polp->Offset += polp->InternalHigh;
        cmd_buf.append(foi->chBuff, polp->InternalHigh);
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
    }
    else if(dwErr != ERROR_HANDLE_EOF)
        displayErrorToCerr("CompletedReadRoutine", polp->Internal);
    else // dwErr == ERROR_HANDLE_EOF
        last_run_apc_func &= ~APC_FUNC_ReadCommand;
}

static void start_apc_func()
{
    if((last_run_apc_func & APC_FUNC_ReadCommand) && !file_read.eof)
    {
        file_read.oOverlap.InternalHigh = 0;
        file_read.oOverlap.Internal = 0;
        ReadFileEx(file_read.hFileHandle, file_read.chBuff, FILE_ASYN_BUF_SIZE, &file_read.oOverlap, CompletedReadRoutine);
    }
    else if(last_run_apc_func & APC_FUNC_Dicomdir)
    {
        QueueUserAPC(MakeDicomdir, GetCurrentThread(), (ULONG_PTR)&last_run_apc_func);
    }
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
    HANDLE tail = CreateFile("cmove.txt", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_NORMAL, NULL);
    if(tail == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("cmove.txt", GetLastError());
        return -2;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();
    cmd_buf.clear();
    cmd_buf.reserve(256);

    memset(&file_read, 0, sizeof(FILE_OLP_INST));
    file_read.hFileHandle = tail;
    if(!ReadFileEx(tail, file_read.chBuff, FILE_ASYN_BUF_SIZE, (OVERLAPPED*)&file_read, CompletedReadRoutine))
    {
        displayErrorToCerr("ReadFileEx", GetLastError());
        return -3;
    }
    
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

    size_t worker_num = 0, queue_size = 0;
    HANDLE *objs = NULL;
    objs = get_worker_handles(&worker_num, &queue_size);
    while(!file_read.eof || (worker_num + queue_size))
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
            last_run_apc_func = APC_FUNC_ALL;
        else if(wr == WAIT_IO_COMPLETION)
            ;
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + worker_num)
        {
            if(complete_worker(wr, objs, worker_num))
                last_run_apc_func |= APC_FUNC_Dicomdir;
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("WaitForMultipleObjectsEx() ", GetLastError());
            if(objs) delete[] objs;
            return -1;
        }

        // push command to compress queue
        while(!queued_cmd.empty())
        {
            const string &cmd = queued_cmd.front();
            if(process_cmd(cmd.c_str()))
                queued_cmd.pop_front();
            else
            {
                file_read.eof = true;
                queued_cmd.pop_front();
                break;
            }
        }
        // if no command, try to push the rest of work to queue
        commit_file_to_workers(NULL);
        if(objs) delete[] objs;
        objs = get_worker_handles(&worker_num, &queue_size);

        // read command, index ...
        start_apc_func();
    }
    if(objs) delete[] objs;
    CloseHandle(tail);
    do {
        MakeDicomdir((ULONG_PTR)&last_run_apc_func);
    } while(last_run_apc_func & APC_FUNC_Dicomdir);
    return 0;
}
