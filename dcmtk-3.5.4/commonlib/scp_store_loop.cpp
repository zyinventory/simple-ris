#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;
static int is_cr_lf(int c) { return (c == '\r' || c == '\n') ? 1 : 0; }
#define STRING_LTRIM(str) str.erase(str.begin(), find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(is_cr_lf))))

bool opt_verbose = false;
const char *sessionId;
HANDLE hDirNotify;

static map<string, bool> inqueue_dfc_files;

static DWORD refresh_files(bool timeout)
{
    WIN32_FIND_DATA wfd;
    char buff[1024] = "state\\*.dfc";
    int pathLen = 4;

    if(timeout && hDirNotify == NULL) return ERROR_BAD_ARGUMENTS;;

    HANDLE hDiskSearch = FindFirstFile(buff, &wfd);
    if(hDiskSearch == INVALID_HANDLE_VALUE) return GetLastError();

    char *buff_ptr = strchr(buff, '\\');
    size_t buff_size = sizeof(buff);
    if(buff_ptr)
    {
        ++buff_ptr;
        buff_size -= (buff_ptr - buff);
    }
    else
    {
        buff_ptr = buff;
    }
    list<string> dfc_files;
    do
	{
        if (strcmp(".", wfd.cFileName) == 0 || strcmp("..", wfd.cFileName) == 0) 
			continue; // skip . ..
        strcpy_s(buff_ptr, buff_size, wfd.cFileName);
        string dfc(buff);
        if(0 == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && inqueue_dfc_files.find(dfc) == inqueue_dfc_files.end())
            dfc_files.push_back(dfc);
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // ¹Ø±Õ²éÕÒ¾ä±ú

    bool end_of_move = false;
    list<string>::iterator it = dfc_files.end();

    for(it = dfc_files.begin(); it != dfc_files.end(); ++it)
    {
        if(opt_verbose) cerr << "refresh_files() loop start: " << *it << endl;
        const char *pos = strrchr(it->c_str(), '_');
        if(pos == NULL)
        {
            cerr << "refresh_files(): file name is not notify: " << *it << endl;
            continue;
        }
        int type = CHAR4_TO_INT(++pos);
        if(type != CHAR4_TO_INT(NOTIFY_STORE_TAG) && type != CHAR4_TO_INT(NOTIFY_MOVE_TAG)
             && type != CHAR4_TO_INT(NOTIFY_FILE_TAG) && type != CHAR4_TO_INT(NOTIFY_ACKN_TAG))
        {
            cerr << "refresh_files(): file type is not notify: " << *it << endl;
            continue;
        }

        ifstream ifcmd(*it, ios_base::in, _SH_DENYRW);
        if(ifcmd.fail())
        {
            if(opt_verbose)
                cerr << "refresh_files(): open file " << *it << " failed, OS close file delay." << endl;
            continue;
        }
        stringstream content;
        do {
            ifcmd.getline(buff, sizeof(buff));
            streamsize read = ifcmd.gcount();
            if(read) content << buff << "\n";
        } while(!ifcmd.eof());

        end_of_move = (process_cmd(content, type, *it) == 0);
        inqueue_dfc_files[*it] = true;
        if(end_of_move) break;
    }

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
        cerr << "make_relate_dir(): mkdir " << dir_name << " faile: " << msg << endl;
        return false;
    }
    return true;
}

static void find_all_study_dir(list<string> &list_studies)
{
    struct _finddata_t wfd;
    intptr_t hSearch = _findfirst("archdir\\*", &wfd);
    if(hSearch == -1)
    {
        perror("find_all_study_dir() failed");
        fprintf_s(stderr, "\tfind %s failed\n", "archdir\\*");
        return;
    }
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0 || node.compare("DICOMDIR") == 0) 
			continue; // skip . .. DICOMDIR
        if(wfd.attrib & _A_SUBDIR) list_studies.push_back(node);
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);
}

static errno_t merge_dir(const char *src, const char *dest)
{
    struct _stat src_stat, dest_stat;
    errno_t en = 0;
    if(_stat(dest, &dest_stat))
    {
        if(errno == ENOENT)
        {
            if(PrepareFileDir(dest))
            {
                if(rename(src, dest))
                {
                    char msg[1024];
                    en = errno;
                    strerror_s(msg, en);
                    cerr << "merge_dir(" << src << ", " << dest << ") dest is not exist, move src failed: " << msg << endl;
                    return en;
                }
                else return 0;
            }
            else
            {
                char msg[1024];
                en = errno;
                strerror_s(msg, en);
                cerr << "merge_dir(" << src << ", " << dest << ") PrepareFileDir(dest) failed: " << msg << endl;
                return en;
            }
        }
        else
        {
            char msg[1024];
            en = errno;
            strerror_s(msg, en);
            cerr << "merge_dir(" << src << ", " << dest << ") dest stat failed: " << msg << endl;
            return en;
        }
    }

    if(_stat(src, &src_stat))
    {
        char msg[1024];
        en = errno;
        strerror_s(msg, en);
        cerr << "merge_dir(" << src << ", " << dest << ") src stat failed: " << msg << endl;
        return en;
    }

    if(src_stat.st_mode & _S_IFDIR)
    {
        if(dest_stat.st_mode & _S_IFDIR) // dest is dir
        {
            // todo: robocopy merge dir
            PROCESS_INFORMATION procinfo;
	        STARTUPINFO sinfo;
	        memset(&sinfo, 0, sizeof(STARTUPINFO));
	        sinfo.cb = sizeof(STARTUPINFO);
            memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));
            char cmd[1024];
            sprintf_s(cmd, "C:\\windows\\system32\\robocopy.exe %s %s /MT /MOVE /S", src, dest);
            if( CreateProcess(NULL, cmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &sinfo, &procinfo) )
            {
                DWORD dw = WaitForSingleObject(procinfo.hProcess, INFINITE);
                if(dw == WAIT_FAILED)
                {
                    DWORD gle = GetLastError();
                    en = EINVAL;
                    displayErrorToCerr(__FUNCSIG__" WaitForSingleObject() failed", gle, &cerr);
                }
                CloseHandle(procinfo.hThread);
                CloseHandle(procinfo.hProcess);
            }
            else
            {
                DWORD gle = GetLastError();
                en = EINVAL;
                char msg[1024];
                sprintf_s(msg, __FUNCSIG__" CreateProcess(%s) failed", cmd);
                displayErrorToCerr(msg, gle, &cerr);
            }
            return en;
        }
        else // dest is file
        {
            if(_unlink(dest)) // delete dest file
            {
                char msg[1024];
                en = errno;
                strerror_s(msg, en);
                cerr << "merge_dir(" << src << ", " << dest << ") src is dir, dest is file, delete dest file failed: " << msg << endl;
                return en;
            }
            else
            {
                if(rename(src, dest))
                {
                    char msg[1024];
                    en = errno;
                    strerror_s(msg, en);
                    cerr << "merge_dir(" << src << ", " << dest << ") dest file is deleted, move src dir failed: " << msg << endl;
                    return en;
                }
                else return 0;
            }
        }
    }
    else // src is file
    {
        if(dest_stat.st_mode & _S_IFDIR) // dest is dir
        {
            if(DeleteTree(dest, &cerr))
            {
                if(rename(src, dest))
                {
                    char msg[1024];
                    en = errno;
                    strerror_s(msg, en);
                    cerr << "merge_dir(" << src << ", " << dest << ") dest dir is deleted, move src file failed: " << msg << endl;
                    return en;
                }
                else return 0;
            }
            else
            {
                cerr << "merge_dir(" << src << ", " << dest << ") dest is dir, delete failed" << endl;
                return EACCES;
            }
        }
        else // dest is file
        {
            if(_unlink(dest)) // delete dest file
            {
                char msg[1024];
                en = errno;
                strerror_s(msg, en);
                cerr << "merge_dir(" << src << ", " << dest << ") dest is file, delete failed: " << msg << endl;
                return en;
            }
            else
            {
                if(rename(src, dest))
                {
                    char msg[1024];
                    en = errno;
                    strerror_s(msg, en);
                    cerr << "merge_dir(" << src << ", " << dest << ") dest file is deleted, move src file failed: " << msg << endl;
                    return en;
                }
                else return 0;
            }
        }
    }
}

static void merge_study_archdir(const list<string> &list_studies, map<string, LARGE_INTEGER> &map_move_study_status)
{
    for(list<string>::const_iterator it = list_studies.begin(); it != list_studies.end(); ++it)
    {
        LARGE_INTEGER state;
        state.HighPart = 0; // for test, todo: find old archive vol.
        state.LowPart = 0;  // last error is 0.
        char prefix[16], src_path[MAX_PATH], dest_path[MAX_PATH];
        HashStr(it->c_str(), prefix, sizeof(prefix));
        sprintf_s(src_path, "archdir\\%s", it->c_str());
        sprintf_s(dest_path, "%s\\pacs\\archdir\\v%07d\\%c%c\\%c%c\\%c%c\\%c%c\\%s", GetPacsBase(), state.HighPart,
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->c_str());

        state.LowPart = merge_dir(src_path, dest_path);

        sprintf_s(dest_path, "%c%c\\%c%c\\%c%c\\%c%c\\%s", 
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->c_str());
        map_move_study_status[dest_path] = state;
    }
}

// move instances to old archive volume
static void overwrite_study_archdir(const list<string> &list_studies, map<string, LARGE_INTEGER> &map_move_study_status)
{
    for(list<string>::const_iterator it = list_studies.begin(); it != list_studies.end(); ++it)
    {
        LARGE_INTEGER state;
        state.HighPart = 0; // for test, todo: find old archive vol.
        state.LowPart = 0;  // last error is 0.
        char prefix[16], src_path[MAX_PATH], dest_path[MAX_PATH];
        HashStr(it->c_str(), prefix, sizeof(prefix));
        sprintf_s(src_path, "archdir\\%s", it->c_str());
        sprintf_s(dest_path, "%s\\pacs\\archdir\\v%07d\\%c%c\\%c%c\\%c%c\\%c%c\\%s", GetPacsBase(), state.HighPart,
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->c_str());
        
        // move study dir 
        bool study_moved = false;
        if(DeleteTree(dest_path, &cerr))
        {
            if(PrepareFileDir(dest_path))
            {
                if(rename(src_path, dest_path))
                {
                    perror("overwrite_study_archdir() rename study dir failed");
                    cerr << "\t" << src_path << " -> " << dest_path << endl;
                }
                else
                {
                    study_moved = true;
                    cout << "trigger archive " << *it << " " << dest_path << endl;

                    // send archive ok notification
                    char notify_file_name[MAX_PATH];
                    int seq_len = in_process_sequence(notify_file_name, sizeof(notify_file_name), STATE_DIR);
                    sprintf_s(notify_file_name + seq_len, sizeof(notify_file_name) - seq_len, "_%s.dfc", NOTIFY_ACKN_TAG);
                    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out);
                    if(ntf.good())
                    {
                        ntf << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ARCHIVE_STUDY 
                            << " " << *it << " " << dest_path << endl;
                        ntf.close();
                    }
                    else
                        cerr << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ARCHIVE_STUDY 
                            << " " << *it << " " << dest_path << endl;
                }
            }
            else
                cerr << "overwrite_study_archdir() can't PrepareFileDir(" << dest_path << ")" << endl;
        }
        else
            cerr << "overwrite_study_archdir() can't delete dir " << dest_path << endl;

        if(!study_moved)
        {
            state.LowPart = ERROR_PATH_NOT_FOUND;
            goto report_study_status;
        }

report_study_status:
        sprintf_s(dest_path, "%c%c\\%c%c\\%c%c\\%c%c\\%s", 
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->c_str());
        map_move_study_status[dest_path] = state;
    }
}

template<class Pred> static void find_recursive(const string &input_path, list<string> &collector, Pred pred)
{
    string path(input_path);
    if(path.empty()) path = ".";
    string::size_type old_size = path.size();
    string::iterator it = path.end();
    --it;
    if(*it != '\\') path.append(1, '\\');
    path.append(1, '*');
    it = path.end();
    --it; // it -> '*'

    struct __finddata64_t wfd;
    intptr_t hSearch = _findfirst64(path.c_str(), &wfd);
    if(hSearch == -1)
    {
        perror("find_recursive() failed");
        fprintf_s(stderr, "\tfind %s failed\n", path.c_str());
        return;
    }
    path.erase(it);
    do {
        if(wfd.attrib & _A_SUBDIR)
        {
            if(strcmp(wfd.name, ".") == 0 || strcmp(wfd.name, "..") == 0) continue;
            size_t old_len = path.length();
            path.append(wfd.name);
            find_recursive(path, collector, pred);
            path.resize(old_len);
        }
        else  // file
        {
            if(pred(&wfd))
            {
                size_t old_len = path.length();
                path.append(wfd.name);
                collector.push_back(path);
                path.resize(old_len);
            }
        }
	} while(_findnext64(hSearch, &wfd) == 0);
	_findclose(hSearch);
}

static bool is_ext_file(const struct __finddata64_t *wfd, const char *ext)
{
    if((wfd->attrib & _A_SUBDIR) == 0)
    {
        const char * p = strstr(wfd->name, ext);
        if(p && strcmp(p, ext) == 0) return true;
    }
    return false;
}

static void find_recursive_xml_file(string &input_path, list<string> &collector)
{
    find_recursive(input_path, collector, bind2nd(ptr_fun(is_ext_file), ".xml"));
}

static void move_index_receive(map<string, errno_t> &map_receive_index)
{
    list<string> collector;
    string path("indexdir\\receive");
    //find_recursive(path, collector, [](const struct __finddata64_t *wfd) { return is_ext_file(wfd, ".xml"); });
    find_recursive_xml_file(path, collector);

    for(list<string>::iterator it = collector.begin(); it != collector.end(); ++it)
    {
        errno_t move_state = 0;
        char dest_path[MAX_PATH];
        sprintf_s(dest_path, "%s\\pacs\\%s", COMMONLIB_PACS_BASE, it->c_str());
        if(PrepareFileDir(dest_path))
        {
            if(_unlink(dest_path) == 0 || errno == ENOENT)
            {
                if(rename(it->c_str(), dest_path))
                {
                    char msg[1024];
                    move_state = errno;
                    strerror_s(msg, move_state);
                    cerr << "move_index_receive() move " << *it << " to " << dest_path << " failed: " << msg << endl;
                }
                else
                {
                    cout << "trigger index_receive " << dest_path << endl;
                }
            }
            else
            {
                char msg[1024];
                move_state = errno;
                strerror_s(msg, move_state);
                cerr << "move_index_receive() delete " << dest_path << " failed: " << msg << endl;
            }
        }
        else
        {
            move_state = EINVAL;
            cerr << "move_index_receive() can't mkdir " << dest_path << endl;
        }
        map_receive_index[*it] = move_state;
    }
}

COMMONLIB_API int scp_store_main_loop(const char *sessId, bool verbose)
{
    opt_verbose = verbose;
    sessionId = sessId;

    if(!make_relate_dir("state")) return -1;
    if(!make_relate_dir("archdir")) return -1;
    if(!make_relate_dir("indexdir")) return -1;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
    
    string fn(_getcwd(NULL, 0));
    fn.append("\\state");
    hDirNotify = FindFirstChangeNotification(fn.c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    if(hDirNotify == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("scp_store_main_loop(): FindFirstChangeNotification()", GetLastError());
        return -2;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();
    
    DWORD gle = 0;
    
    gle = NamedPipe_CreateListening(sessionId, false);
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        NamedPipe_CloseHandle(true);
        CloseHandle(hDirNotify);
        displayErrorToCerr("scp_store_loop() NamedPipe_CreateListening() error", gle);
        return -4;
    }
    gle = NamedPipe_CreateClientProc(".");
    if(gle)
    {
        NamedPipe_CloseHandle(true);
        CloseHandle(hDirNotify);
        displayErrorToCerr("scp_store_loop() NamedPipe_CreateClientProc(.) error", gle);
        return -5;
    }
    
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

    // hPipeEvent must be alive, hDirNotify is the first exit signal.
    // so worker_num must be 1, all_queue_size must be 0.
    while(worker_num + all_queue_size > 1)
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
                    cout << "trigger ready_close" << endl;
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
                cerr << "scp_store_main_loop(): fatal error at worker_complete()" << endl;
                break;
            }
        }
        else
        {   // shall not reach here ...
            displayErrorToCerr("scp_store_main_loop(): WaitForMultipleObjectsEx() ", GetLastError());
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

    // send compress and dicomdir ok notification
    char notify_file_name[MAX_PATH];
    int seq_len = in_process_sequence(notify_file_name, sizeof(notify_file_name), STATE_DIR);
    sprintf_s(notify_file_name + seq_len, sizeof(notify_file_name) - seq_len, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out);
    if(ntf.good())
    {
        ntf << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;
        ntf.close();
    }
    else
        cerr << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;

    save_index_study_receive_to_session();

    list<string> list_studies;
    find_all_study_dir(list_studies);

    // LARGE_INTEGER: HighPart is volume id, LowPart is last error state.
    map<string, LARGE_INTEGER> map_move_study_status;
    merge_study_archdir(list_studies, map_move_study_status);

    merge_index_study_patient_date(true, map_move_study_status);

    map<string, errno_t> map_receive_index;
    move_index_receive(map_receive_index);
    for_each(map_receive_index.begin(), map_receive_index.end(), [](const pair<string, errno_t> &p)
    {
        if(p.second)
            cerr << "scp_store_main_loop() move_index_receive() error " << p.second << " at " << p.first << endl;
    });

    // send xml ok notification
    seq_len = in_process_sequence(notify_file_name, sizeof(notify_file_name), STATE_DIR);
    sprintf_s(notify_file_name + seq_len, sizeof(notify_file_name) - seq_len, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf_final(notify_file_name, ios_base::trunc | ios_base::out);
    if(ntf_final.good())
    {
        ntf_final << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_XML_OK << endl;
        ntf_final.close();
    }
    else
        cerr << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_XML_OK << endl;

    CoUninitialize();
#ifdef _DEBUG
    //DeleteSubTree("archdir");
    //DeleteSubTree("indexdir");
    //DeleteSubTree("state");
#endif
    return gle;
}
