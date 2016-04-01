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
static list<string> delay_dfc;

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
	FindClose(hDiskSearch); // 关闭查找句柄

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
        if(opt_verbose) cerr << "refresh_files() loop start: " << *it << endl;
        if(end_of_move && strstr(it->c_str(), "_N.dfc"))
        {
            if(opt_verbose) cerr << "refresh_files(): ignore notify " << *it << " after end of move." << endl;
            continue;  // skip notify file after end_of_move
        }
        list<string>::iterator dlit = find_if(delay_dfc.begin(), delay_dfc.end(),
            [&it](const string &dlfn) { return it->compare(dlfn) == 0; });

        strcpy_s(buff_ptr, buff_size, it->c_str());
        ifstream ifcmd(fileFilter, ios_base::in, _SH_DENYRW);
        if(ifcmd.fail())
        {
            if(opt_verbose) cerr << "refresh_files(): open file " << fileFilter << " failed, OS close file delay." << endl;
            if(dlit == delay_dfc.end()) delay_dfc.push_back(*it);
            break;
        }
        else if(dlit != delay_dfc.end())
        {
            if(opt_verbose) cerr << "refresh_files(): retry file " << fileFilter << " OK." << endl;
            delay_dfc.erase(dlit);
        }

        char cmd[1024];
        ifcmd.getline(cmd, sizeof(cmd));
        while(!ifcmd.fail())
        {
            if(strlen(cmd))
            {
                if(end_of_move)
                    fprintf_s(stderr, "Commands is after end-of-move: %s\n", cmd);
                else if(0 == process_cmd(cmd))
                    end_of_move = true;
            }
            ifcmd.getline(cmd, sizeof(cmd));
        }
        ifcmd.close();
        last_dfc = *it;
    }

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
        cerr << "make_relate_dir(): mkdir " << dir_name << " faile: " << msg << endl;
        return false;
    }
    return true;
}

static void find_all_study_dir(map<string, string> &map_studies_dicomdir)
{
    list<string> study_dirs, dir_files;
    struct _finddata_t wfd;
    intptr_t hSearch = _findfirst("archdir\\*", &wfd);
    if(hSearch == -1)
    {
        perror("move_study_dir() failed");
        fprintf_s(stderr, "\tfind %s failed\n", "archdir\\*");
        return;
    }
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0 || node.compare("DICOMDIR") == 0) 
			continue; // skip . .. DICOMDIR
        if(wfd.attrib & _A_SUBDIR)
            study_dirs.push_back(node);
        else
            dir_files.push_back(node);
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);

    for(list<string>::iterator it_study = study_dirs.begin(); it_study != study_dirs.end(); ++it_study)
    {
        string dir_filename(*it_study);
        dir_filename.append(".dir");
        list<string>::iterator it_dicomdir = find_if(dir_files.begin(), dir_files.end(),
            [&dir_filename](const string &fn) { return fn.compare(dir_filename) == 0; });
        if(it_dicomdir != dir_files.end())
        {
            map_studies_dicomdir[*it_study] = dir_filename;
            dir_files.erase(it_dicomdir);
        }
        else
            cerr << "study " << *it_study << " can't find matched DICOMDIR " << dir_filename << endl;
    }
    for_each(dir_files.begin(), dir_files.end(), [](const string &fn) {
        cerr << "dicomdir " << fn << " remain, there is no matched study UID." << endl; 
    });
}

// todo: for storescp, merge instances to old archive volume

// move instances to old archive volume
static void overwrite_study_archdir(const char *pacs_base, const map<string, string> &map_studies_dicomdir, map<string, LARGE_INTEGER> &map_move_study_status)
{
    for(map<string, string>::const_iterator it = map_studies_dicomdir.begin(); it != map_studies_dicomdir.end(); ++it)
    {
        LARGE_INTEGER state;
        state.HighPart = 0; // for test, todo: find old archive vol.
        state.LowPart = 0;  // last error is 0.
        char prefix[16], src_path[MAX_PATH], dest_path[MAX_PATH];
        HashStr(it->first.c_str(), prefix, sizeof(prefix));
        sprintf_s(src_path, "archdir\\%s", it->first.c_str());
        sprintf_s(dest_path, "%s\\pacs\\archdir\\v%07d\\%c%c\\%c%c\\%c%c\\%c%c\\%s", pacs_base, state.HighPart,
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->first.c_str());
        
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
                    cout << "trigger archive " << it->first << " " << dest_path << endl;

                    // send archive ok notification
                    char notify_file_name[MAX_PATH];
                    GetNextUniqueNo("state\\", notify_file_name, sizeof(notify_file_name));
                    strcat_s(notify_file_name, "_N.dfc");
                    ofstream ntf(notify_file_name, ios_base::app | ios_base::out);
                    if(ntf.good())
                    {
                        ntf << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_ARCHIVE 
                            << " " << it->first << " " << dest_path << endl;
                        ntf.close();
                    }
                    else
                        cerr << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_ARCHIVE 
                            << " " << it->first << " " << dest_path << endl;
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

        // move dicomdir
        bool dicomdir_moved = false;
        strcat_s(src_path, ".dir");
        strcat_s(dest_path, ".dir");
        errno_t ec = _unlink(dest_path);
        if(ec == 0 || errno == ENOENT)
        {
            if(rename(src_path, dest_path))
            {
                perror("overwrite_study_archdir() rename dicomdir failed");
                cerr << "\t" << src_path << " -> " << dest_path << endl;
            }
            else
            {
                dicomdir_moved = true;
                cout << "trigger dicomdir " << it->first << ".dir " << dest_path << endl;

                // send dicomdir ok notification
                char notify_file_name[MAX_PATH];
                GetNextUniqueNo("state\\", notify_file_name, sizeof(notify_file_name));
                strcat_s(notify_file_name, "_N.dfc");
                ofstream ntf(notify_file_name, ios_base::app | ios_base::out);
                if(ntf.good())
                {
                    ntf << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_DICOMDIR 
                        << " " << it->first << " " << dest_path << endl;
                    ntf.close();
                }
                else
                    cerr << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_DICOMDIR 
                        << " " << it->first << " " << dest_path << endl;
            }
        }
        else
            cerr << "overwrite_study_archdir() can't delete dicomdir " << dest_path << endl;

        if(!dicomdir_moved) state.LowPart = ERROR_FILE_NOT_FOUND;

report_study_status:
        sprintf_s(dest_path, "%c%c\\%c%c\\%c%c\\%c%c\\%s", 
            prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5], prefix[6], prefix[7], it->first.c_str());
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

static void move_index_receive(const char *pacs_base, map<string, errno_t> &map_receive_index)
{
    list<string> collector;
    string path("indexdir\\receive");
    //find_recursive(path, collector, [](const struct __finddata64_t *wfd) { return is_ext_file(wfd, ".xml"); });
    find_recursive_xml_file(path, collector);

    for(list<string>::iterator it = collector.begin(); it != collector.end(); ++it)
    {
        errno_t move_state = 0;
        char dest_path[MAX_PATH];
        sprintf_s(dest_path, "%s\\pacs\\%s", pacs_base, it->c_str());
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
    string fn("\\storedir\\");
    fn.append(sessionId);
    if(ChangeToPacsWebSub(pacs_base, MAX_PATH, fn.c_str()))
    {
        cerr << "scp_store_main_loop(): 无法切换工作目录" << endl;
        return -1;
    }
    if(!make_relate_dir("state")) return -1;
    if(!make_relate_dir("archdir")) return -1;
    if(!make_relate_dir("indexdir")) return -1;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
    
    fn = _getcwd(NULL, 0);
    fn.append("\\state");
    hDirNotify = FindFirstChangeNotification(fn.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
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
        return -4;
    }
    gle = NamedPipe_CreateClientProc(".");
    if(gle)
    {
        NamedPipe_CloseHandle(true);
        CloseHandle(hDirNotify);
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

    for(list<string>::iterator it = delay_dfc.begin(); it != delay_dfc.end(); ++it)
    {
        cerr << "scp_store_main_loop(): remain delay file " << *it << endl;
    }

    // send compress and dicomdir ok notification
    char notify_file_name[MAX_PATH];
    GetNextUniqueNo("state\\", notify_file_name, sizeof(notify_file_name));
    strcat_s(notify_file_name, "_N.dfc");
    ofstream ntf(notify_file_name, ios_base::app | ios_base::out);
    if(ntf.good())
    {
        ntf << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPR_OK << endl;
        ntf.close();
    }
    else
        cerr << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPR_OK << endl;

    save_index_study_receive_to_session();

    map<string, string> map_studies_dicomdir;
    find_all_study_dir(map_studies_dicomdir);

    // LARGE_INTEGER: HighPart is volume id, LowPart is last error state.
    map<string, LARGE_INTEGER> map_move_study_status;
    overwrite_study_archdir(pacs_base, map_studies_dicomdir, map_move_study_status);

    merge_index_study_patient_date(pacs_base, true, map_move_study_status);

    map<string, errno_t> map_receive_index;
    move_index_receive(pacs_base, map_receive_index);
    for_each(map_receive_index.begin(), map_receive_index.end(), [](const pair<string, errno_t> &p)
    {
        if(p.second)
            cerr << "scp_store_main_loop() move_index_receive() error " << p.second << " at " << p.first << endl;
    });

    // send xml ok notification
    GetNextUniqueNo("state\\", notify_file_name, sizeof(notify_file_name));
    strcat_s(notify_file_name, "_N.dfc");
    ofstream ntf_final(notify_file_name, ios_base::app | ios_base::out);
    if(ntf_final.good())
    {
        ntf_final << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_XML_OK << endl;
        ntf_final.close();
    }
    else
        cerr << "N "<< hex << setw(8) << setfill('0') << uppercase << NOTIFY_XML_OK << endl;

    CoUninitialize();
#ifdef _DEBUG
    //DeleteSubTree("archdir");
    //DeleteSubTree("indexdir");
    DeleteSubTree("state");
#endif
    return gle;
}
