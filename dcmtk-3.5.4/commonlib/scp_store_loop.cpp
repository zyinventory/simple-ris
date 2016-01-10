#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;
static int is_cr_lf(int c) { return (c == '\r' || c == '\n') ? 1 : 0; }
#define STRING_LTRIM(str) str.erase(str.begin(), find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(is_cr_lf))))

bool opt_verbose = false;
char pacs_base[MAX_PATH];
const char *sessionId;
HANDLE hDirNotify;

static string last_dfc;
static bool close_too_late = false;

static bool refresh_files(bool timeout)
{
    WIN32_FIND_DATA wfd;
    char fileFilter[MAX_PATH] = "log\\*.dfc";
    int pathLen = 4;

    if(timeout && hDirNotify == NULL) return false;

    HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
    list<string> dfc_files;
    do
	{
        string dfc(wfd.cFileName);
        if (dfc.compare(".") == 0 || dfc.compare("..") == 0) 
			continue; // skip . ..
        if(0 == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && dfc.compare(last_dfc) > 0)
            dfc_files.push_back(dfc);
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // 关闭查找句柄

    dfc_files.sort();
    bool end_of_move = false;
    list<string>::iterator it = dfc_files.end();
    for(it = dfc_files.begin(); it != dfc_files.end(); ++it)
    {
        fileFilter[4] = '\0';
        strcat_s(fileFilter, it->c_str());
        ifstream ifcmd(fileFilter, ios_base::in, _SH_DENYRW);
        if(ifcmd.fail())
        {
            if(opt_verbose) cerr << "open file " << fileFilter << " failed, OS close file delay." << endl;
            close_too_late = true;
            break;
        }
        char cmd[1024];
        ifcmd.getline(cmd, sizeof(cmd));
        while(!ifcmd.fail())
        {
            if(strlen(cmd))
            {
                if(end_of_move)
                {
                    HANDLE herr = CreateFile("cmove_error.txt",  FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if(herr == INVALID_HANDLE_VALUE)
                        displayErrorToCerr("CreateFile(cmove_error.txt)", GetLastError());
                    else
                    {
                        DWORD written = 0;
                        char *tip = new char[1024];
                        size_t tiplen = sprintf_s(tip, 1024, "Commands is after end-of-move: %s\r\n", cmd);
                        if(!WriteFile(herr, tip, tiplen, &written, NULL))
                            displayErrorToCerr("WriteFile() to cmove_error.txt", GetLastError());
                        delete[] tip;
                        CloseHandle(herr);
                    }
                }
                else if(0 == process_cmd(cmd)) end_of_move = true;
            }
            ifcmd.getline(cmd, sizeof(cmd));
        }
        ifcmd.close();
        last_dfc = *it;
    }
    if(it == dfc_files.end() && !dfc_files.empty()) close_too_late = false;

    if(timeout) return !dfc_files.empty();

    if(end_of_move)
    {
        FindCloseChangeNotification(hDirNotify);
        hDirNotify = NULL;
        return false;
    }
    else
    {
        FindNextChangeNotification(hDirNotify);
        return true;
    }
}

static bool read_cmd_continous(HANDLE)
{
    return refresh_files(false);
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
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir log faile: " << msg << endl;
        return -1;
    }
    if(_mkdir("archdir") && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir archdir faile: " << msg << endl;
        return -1;
    }
    
    fn = _getcwd(NULL, 0);
    fn.append("\\log");
    hDirNotify = FindFirstChangeNotification(fn.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hDirNotify == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("FindFirstChangeNotification()", GetLastError());
        return -2;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();
    
    DWORD gle = NamedPipe_CreateListening();
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        NamedPipe_CloseHandle(true);
        CloseHandle(hDirNotify);
        return -4;
    }
    gle = NamedPipe_CreateClientProc(".");
    if(gle)
    {
        NamedPipe_CloseHandle(true);
        CloseHandle(hDirNotify);
        return -5;
    }

    size_t worker_num = 0, queue_size = 0;
    HANDLE *objs = NULL;
    WORKER_CALLBACK *cbs = NULL;
    objs = get_worker_handles(&worker_num, &queue_size, &cbs, hDirNotify ? 1 : 0);
    if(hDirNotify)
    {
        objs[0] = hDirNotify;
        cbs[0] = read_cmd_continous;
    }

    while(worker_num + queue_size > 1) //hEventPipe must be alive, hDirNotify is the first exit signal
    {   // WaitForMultipleObjectsEx() timeout must be short time,
        // dcmmkdir_sleep_time / dcmmkdir_proc_num > WaitForMultipleObjectsEx(timeout)
        // otherwise dcmmkdir 's again will always trigger WAIT_IO_COMPLETION.
        DWORD wr = WaitForMultipleObjectsEx(worker_num, objs, FALSE, 200, TRUE);
        // switch(wr)
        if(wr == WAIT_TIMEOUT)
        {
            if(hDirNotify)
                refresh_files(true);
            else if(!ready_to_close_dcmmkdir_workers)
            {
                if(is_idle()) // no more work
                {
                    ready_to_close_dcmmkdir_workers = true;
                    cerr << "trigger ready_close" << endl;
                }
            }
            else
            {
                // active all blocked dcmmkdir, then they will term.
                close_all_blocked_pipe_instances();
            }
        }
        else if(wr == WAIT_IO_COMPLETION)
            ;
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + worker_num)
        {
            worker_complete(wr, objs, cbs, worker_num);
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("WaitForMultipleObjectsEx() ", GetLastError());
            break;
        }

        if(objs) delete[] objs;
        if(cbs) delete[] cbs;
        objs = get_worker_handles(&worker_num, &queue_size, &cbs, hDirNotify ? 1 : 0);
        if(hDirNotify)
        {
            objs[0] = hDirNotify;
            cbs[0] = read_cmd_continous;
        }
    }
    if(objs) delete[] objs;
    if(cbs) delete[] cbs;
    NamedPipe_CloseHandle(true);
    return 0;
}
