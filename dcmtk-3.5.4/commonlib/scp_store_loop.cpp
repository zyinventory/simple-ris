#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;
static int is_cr_lf(int c) { return (c == '\r' || c == '\n') ? 1 : 0; }
#define STRING_LTRIM(str) str.erase(str.begin(), find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(is_cr_lf))))

bool opt_verbose = false;
char pacs_base[MAX_PATH];
const char *sessionId;

static HANDLE hDirNotify;
static ofstream strmdup;
static string last_dfc;

static bool read_cmd_continous()
{
    WIN32_FIND_DATA wfd;
    char fileFilter[MAX_PATH] = "log\\*.dfc";
    int pathLen = 4;
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
    if(dfc_files.empty())
        cerr << "######################" << endl;
    for(list<string>::iterator it = dfc_files.begin(); it != dfc_files.end(); ++it)
    {
        fileFilter[4] = '\0';
        strcat_s(fileFilter, it->c_str());
        ifstream ifcmd(fileFilter, ios_base::in, _SH_DENYRW);
        if(ifcmd.fail())
        {
            cerr << "open file " << fileFilter << " failed" << endl;
            break;
        }
        char cmd[1024];
        ifcmd.getline(cmd, sizeof(cmd));
        while(!ifcmd.fail())
        {
            if(strlen(cmd) == 0)
            {
                continue;
            }
            strmdup << cmd << endl;
            if(!end_of_move && 0 == process_cmd(cmd)) end_of_move = true;
            ifcmd.getline(cmd, sizeof(cmd));
        }
        ifcmd.close();
        last_dfc = *it;
    }

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
    
    strmdup.open("cmove_dup.txt", ios_base::trunc);
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
    objs = get_worker_handles(&worker_num, &queue_size, &cbs, hDirNotify ? 1 : 0);
    if(hDirNotify)
    {
        objs[0] = hDirNotify;
        cbs[0] = read_cmd_continous;
    }

    while(worker_num + queue_size)
    {
        DWORD wr = WAIT_FAILED;
        if(objs) wr = WaitForMultipleObjectsEx(worker_num, objs, FALSE, 100, TRUE);
        else
        {
            wr = SleepEx(100, TRUE);
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
            complete_worker(wr, objs, cbs, worker_num);
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("WaitForMultipleObjectsEx() ", GetLastError());
            if(objs) delete[] objs;
            return -1;
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
    if(strmdup.is_open()) strmdup.close();
    return 0;
}
