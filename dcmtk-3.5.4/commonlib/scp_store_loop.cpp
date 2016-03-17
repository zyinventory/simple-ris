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

static DWORD refresh_files(bool timeout)
{
    WIN32_FIND_DATA wfd;
    char fileFilter[MAX_PATH] = "state\\*.dfc";
    int pathLen = 4;

    if(timeout && hDirNotify == NULL) return ERROR_BAD_ARGUMENTS;;

    HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
    if(hDiskSearch == INVALID_HANDLE_VALUE) return GetLastError();
    list<string> dfc_files;
    do
	{
        string dfc(wfd.cFileName);
        if (dfc.compare(".") == 0 || dfc.compare("..") == 0) 
			continue; // skip . ..
        if(0 == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && dfc.compare(last_dfc) > 0)
            dfc_files.push_back(dfc);
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // �رղ��Ҿ��

    dfc_files.sort();
    bool end_of_move = false;
    list<string>::iterator it = dfc_files.end();

    char *buff_ptr = strchr(fileFilter, '\\');
    size_t buff_size = sizeof(fileFilter);
    if(buff_ptr)
    {
        ++buff_ptr;
        buff_size -= (buff_ptr - fileFilter);
    }
    else
    {
        buff_ptr = fileFilter;
    }

    for(it = dfc_files.begin(); it != dfc_files.end(); ++it)
    {
        strcpy_s(buff_ptr, buff_size, it->c_str());
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

    if(timeout) return ERROR_SUCCESS;

    if(end_of_move)
    {
        FindCloseChangeNotification(hDirNotify);
        hDirNotify = NULL;
    }
    else
    {
        if(!FindNextChangeNotification(hDirNotify))
            return displayErrorToCerr("refresh_files() FindNextChangeNotification()", GetLastError());
    }
    return ERROR_SUCCESS;
}

static DWORD read_cmd_continous(HANDLE)
{
    return refresh_files(false);
}

static bool make_relate_dir(const char *dir_name)
{
    if(_mkdir(dir_name) && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir " << dir_name << " faile: " << msg << endl;
        return false;
    }
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
        cerr << "�޷��л�����Ŀ¼" << endl;
        return -1;
    }
    if(!make_relate_dir("state")) return -1;
    if(!make_relate_dir("archdir")) return -1;
    if(!make_relate_dir("indexdir")) return -1;
    bool com_init = (CoInitialize(NULL) == S_OK);
    if(!com_init) return -1;
    
    fn = _getcwd(NULL, 0);
    fn.append("\\state");
    hDirNotify = FindFirstChangeNotification(fn.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hDirNotify == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("FindFirstChangeNotification()", GetLastError());
        return -2;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();
    
    DWORD gle = 0;
    /*
    gle = NamedPipe_CreateListening(sessionId, false);
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
    */
    size_t worker_num = 0, all_queue_size = 0;
    HANDLE *objs = NULL;
    WORKER_CALLBACK *cbs = NULL;
    // worker_num is number_of_compress_workers + number_of_dcmmkdir_workers + reserve
    // worker_num is elements of objs
    // all_queue_size is size_of_compress_queue + size_of_dir_queue
    objs = get_worker_handles(&worker_num, &all_queue_size, &cbs, hDirNotify ? 1 : 0);
    if(hDirNotify)
    {
        objs[0] = hDirNotify;
        cbs[0] = read_cmd_continous;
    }

    // hEventPipe must be alive, hDirNotify is the first exit signal.
    // so worker_num must be 1, all_queue_size must be 0.
    while(worker_num + all_queue_size > 0)
    {
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
            gle = worker_complete(wr, objs, cbs, worker_num);
            if(gle)
            {
                cerr << "fatal error at worker_complete()" << endl;
                break;
            }
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("WaitForMultipleObjectsEx() ", GetLastError());
            break;
        }

        if(objs) delete[] objs;
        if(cbs) delete[] cbs;
        objs = get_worker_handles(&worker_num, &all_queue_size, &cbs, hDirNotify ? 1 : 0);
        if(hDirNotify)
        {
            objs[0] = hDirNotify;
            cbs[0] = read_cmd_continous;
        }
    }
    if(objs) delete[] objs;
    if(cbs) delete[] cbs;
    NamedPipe_CloseHandle(true);
    clear_map();
    if(com_init) CoUninitialize();

    WIN32_FIND_DATA wfd;
    HANDLE hDiskSearch = FindFirstFile("state\\*.dfc", &wfd);
    if(hDiskSearch != INVALID_HANDLE_VALUE)
    {
        do
	    {
            string dfc(wfd.cFileName);
            if (dfc.compare(".") == 0 || dfc.compare("..") == 0) 
			    continue; // skip . ..
            if(0 == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                string to_del("state\\");
                to_del.append(dfc);
                remove(to_del.c_str());
            }
	    } while (FindNextFile(hDiskSearch, &wfd));
	    FindClose(hDiskSearch); // �رղ��Ҿ��
    }
    return gle;
}
