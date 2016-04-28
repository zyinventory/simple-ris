#include "stdafx.h"
#include "commonlib.h"

using namespace std;

char exec_cmd[1024];

static char buff[1024];
static PROCESS_INFORMATION qr_procinfo, merge_procinfo;
static list<string> notify_list, notify_merged_list;
static string transfer_dir, current_notify_file;
static list<string> transfer_notify_files, transfer_notify_processed_files;

static int create_child_proc(char *cmd, const char *exec_name, const char *cwd_path, PPROCESS_INFORMATION pprocinfo, ostream &flog)
{
	STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

	SECURITY_ATTRIBUTES logSA;
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE logFile = INVALID_HANDLE_VALUE;
	size_t pos = GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_", buff, sizeof(buff));
    sprintf_s(buff + pos, sizeof(buff) - pos, "%s.txt", exec_name);

	if(PrepareFileDir(buff))
		logFile = CreateFile(buff, GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if(logFile != INVALID_HANDLE_VALUE)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdOutput = logFile;
		sinfo.hStdError = logFile;
		sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	}

	if( CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, cwd_path, &sinfo, pprocinfo) )
	{
        CloseHandle(logFile);
        if(opt_verbose) time_header_out(flog) << "create_child_proc(" << cmd << ") OK" << endl;
		return 0;
	}
	else
	{
        DWORD gle = GetLastError();
        CloseHandle(logFile);
		return gle;
	}
}

static int find_files(const char *filter, list<string> &filelist, list<string> &mergedlist, ostream &flog)
{
    struct _finddata_t wfd;
    intptr_t hSearch = _findfirst(filter, &wfd);
    if(hSearch == -1)
    {
        errno_t en = errno;
        strerror_s(buff, en);
        if(opt_verbose || en != ENOENT) time_header_out(flog) << "find_files() " << filter << " failed: " << buff << endl;
        return -1;
    }
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0) 
			continue; // skip . .. DICOMDIR
        if((wfd.attrib & _A_SUBDIR) == 0)
        {
            if(find(filelist.begin(), filelist.end(), node) == filelist.end()
                && find(mergedlist.begin(), mergedlist.end(), node) == mergedlist.end())
                filelist.push_back(node);
        }
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);
    filelist.sort();
    return filelist.size();
}

// if notify compress and index OK, return true, erease it;
// else remain it.
static DWORD process_notify(const string &notify_file, string &transfer_path, ostream &flog)
{
    string ntffn("store_notify\\");
    ntffn.append(notify_file);
    if(opt_verbose) time_header_out(flog) << "receive association notify " << notify_file << endl;
#ifdef _DEBUG
    time_header_out(cerr) << "receive association notify " << notify_file << endl;
#endif
    ifstream ntff(ntffn);
    if(ntff.fail()) return GetLastError();
    string cmd, path, assoc_id, calling, called, remote, port;
    DWORD tag, pid;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port;
    if(ntff.is_open()) ntff.close();
#ifdef _DEBUG
    time_header_out(cerr) << cmd << " " << hex << tag << " " << path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;
#endif
    if(path[1] != ':') path.insert(0, "\\pacs\\").insert(0, GetPacsBase());
    transfer_path = path;
    string::size_type path_pos = path.find_last_of('\\');
    string association_id = path.substr(path_pos + 1);
    path.erase(path_pos);
    
#ifdef _DEBUG
    strcpy_s(exec_cmd, __FILE__);
    char *p = strrchr(exec_cmd, '\\');
    if(p) *p = '\0'; // cd ..
    p = strrchr(exec_cmd, '\\');
    if(p) *p = '\0'; // cd ..
    if(p) sprintf_s(p, sizeof(exec_cmd) - (p - exec_cmd), "\\Debug\\mergedir.exe %s %s", (opt_verbose ? "-v" : ""), association_id.c_str());
    else sprintf_s(exec_cmd, "..\\bin\\mergedir.exe %s %s", (opt_verbose ? "-v" : ""), association_id.c_str());
#else
    sprintf_s(exec_cmd, "..\\bin\\mergedir.exe %s %s", (opt_verbose ? "-v" : ""), association_id.c_str());
#endif
    return create_child_proc(exec_cmd, "mergedir", path.c_str(), &merge_procinfo, flog);
}

DWORD process_merge_notify(bool lastCall, HANDLE hTransferDir, ostream &flog)
{
    if(opt_verbose)
    {
        time_header_out(flog) << "------ ";
        if(lastCall) flog << "last    ";
        else flog <<              "continue";
        flog << " call ------" << endl;
    }
#ifdef _DEBUG
        time_header_out(cerr) << "------ ";
        if(lastCall) cerr << "last    ";
        else cerr <<              "continue";
        cerr << " call ------" << endl;
#endif
    string transfer_dir_filter(transfer_dir);
    transfer_dir_filter.append(1, '\\');
    size_t prefix_len = transfer_dir_filter.length();
    transfer_dir_filter.append("*.dfc");
    find_files(transfer_dir_filter.c_str(), transfer_notify_files, transfer_notify_processed_files, flog);

    list<string>::iterator it = transfer_notify_files.begin();
    while(it != transfer_notify_files.end())
    {
        transfer_dir_filter.erase(prefix_len);
        transfer_dir_filter.append(*it);
        if(opt_verbose) time_header_out(flog) << "process merge notify " << transfer_dir_filter << endl;
#ifdef _DEBUG
        time_header_out(cerr) << "process merge notify " << transfer_dir_filter << endl;
#endif
        if(read_notify_info(transfer_dir_filter))
        {
            transfer_notify_processed_files.push_back(*it);
            it = transfer_notify_files.erase(it);
        }
        else
        {
            time_header_out(cerr) << "notify file process failed: " << *it << endl;
            ++it;
        }
    }

    if(lastCall)
    {
        if(FindCloseChangeNotification(hTransferDir))
            return 0;
        else
            return displayErrorToCerr("process_merge_notify() FindCloseChangeNotification()", GetLastError(), &flog);
    }
    else
    {
        if(FindNextChangeNotification(hTransferDir))
            return 0;
        else
        {
            DWORD gle = displayErrorToCerr("process_merge_notify() FindNextChangeNotification()", GetLastError(), &flog);
            FindCloseChangeNotification(hTransferDir);
            return gle;
        }
    }
}

#define NOTIFY_FILTER "store_notify\\*.dfc"

int watch_notify(ostream &flog)
{
	memset(&qr_procinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&merge_procinfo, 0, sizeof(PROCESS_INFORMATION));

    DWORD gle = create_child_proc(exec_cmd, "dcmqrscp", NULL, &qr_procinfo, flog);
    if(gle)
    {
        displayErrorToCerr("watch_notify() create_child_proc(dcmqrscp) at beginning", gle, &flog);
        return gle;
    }
#ifdef _DEBUG
    WaitForInputIdle(qr_procinfo.hProcess, INFINITE);
#endif
    SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
    DWORD worker_core_num = max(2, sysInfo.dwNumberOfProcessors - 2);
    HANDLE hSema = CreateSemaphore(NULL, worker_core_num, worker_core_num, "Global\\semaphore_compress_process");
    if(hSema == NULL)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() CreateSemaphore()", gle, &flog);
        if(gle == ERROR_ALREADY_EXISTS)
        {
            time_header_out(flog) << "semaphore has existed, try to open it." << endl;
            hSema = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, "Global\\semaphore_compress_process");
            if(hSema == NULL) displayErrorToCerr("watch_notify() OpenSemaphore()", GetLastError(), &flog);
        }
    }
    
    HANDLE ha[4] = { qr_procinfo.hProcess, NULL, NULL, NULL };
    sprintf_s(buff, "%s\\pacs\\store_notify", GetPacsBase());
    ha[1] = FindFirstChangeNotification(buff, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(ha[1] == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() FindFirstChangeNotification()", gle, &flog);
        goto clean_child_proc;
    }

    // collect exist dfc files
    find_files(NOTIFY_FILTER, notify_list, notify_merged_list, flog);

    while(GetSignalInterruptValue() == 0)
    {
        size_t hcnt = 2;
        if(ha[2] && ha[2] != INVALID_HANDLE_VALUE)
        {
            ++hcnt;
            if(ha[3] && ha[3] != INVALID_HANDLE_VALUE) ++hcnt;
        }
        DWORD wr = WaitForMultipleObjects(hcnt, ha, FALSE, 1000);

        if(wr == WAIT_TIMEOUT)
        {

        }
        else if(wr == WAIT_OBJECT_0) // restart dcmqrscp
        {
            time_header_out(flog) << "dcmqrscp encounter error, restart." << endl;
            if(qr_procinfo.hThread && qr_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hThread);
            if(qr_procinfo.hProcess && qr_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hProcess);
            memset(&qr_procinfo, 0, sizeof(PROCESS_INFORMATION));
            gle = create_child_proc(exec_cmd, "dcmqrscp", NULL, &qr_procinfo, flog);
            if(gle)
            {
                displayErrorToCerr("watch_notify() create_child_proc(dcmqrscp) restart", gle, &flog);
                goto clean_child_proc;
            }
        }
        else if(wr == WAIT_OBJECT_0 + 1) // new file in store_notify
        {
            find_files(NOTIFY_FILTER, notify_list, notify_merged_list, flog);
            if(FALSE == FindNextChangeNotification(ha[1]))
            {
                gle = displayErrorToCerr("watch_notify() FindNextChangeNotification()", GetLastError(), &flog);
                goto clean_child_proc;
            }
        }
        else if(wr == WAIT_OBJECT_0 + 2) // mergedir.exe complete
        {
            if(opt_verbose) time_header_out(flog) << "watch_notify() mergedir.exe complete" << endl;
#ifdef _DEBUG
            time_header_out(cerr) << "watch_notify() mergedir.exe complete" << endl;
#endif
            if(merge_procinfo.hThread && merge_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hThread);
            if(merge_procinfo.hProcess && merge_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hProcess);
            memset(&merge_procinfo, 0, sizeof(PROCESS_INFORMATION));
            ha[2] = NULL;
            // last call process_merge_notify()
            if(ha[3] && ha[3] != INVALID_HANDLE_VALUE) process_merge_notify(true, ha[3], flog);
            ha[3] = NULL;
            transfer_notify_files.clear();
            transfer_notify_processed_files.clear();
            notify_merged_list.remove(current_notify_file);
            current_notify_file.insert(0, "store_notify\\");
            string merged_notify_file(current_notify_file);
            //_unlink(current_notify_file.c_str());
            merged_notify_file.append(".txt");
            rename(current_notify_file.c_str(), merged_notify_file.c_str());
            current_notify_file.clear();
            //DeleteTree(transfer_dir.c_str());
            transfer_dir.clear();
        }
        else if(wr == WAIT_OBJECT_0 + 3) // some file in storedir/association_id
        {
            if(process_merge_notify(false, ha[3], flog)) ha[3] = NULL;
        }
        else
        {
            gle = displayErrorToCerr("watch_notify() WaitForMultipleObjects()", GetLastError(), &flog);
            break;
        }

        if(merge_procinfo.hProcess == 0 || merge_procinfo.hProcess == INVALID_HANDLE_VALUE)
        {   // if mergedir idle, start another mergedir.
            list<string>::iterator it = notify_list.begin();
            while(it != notify_list.end() &&
                (merge_procinfo.hProcess == 0 || merge_procinfo.hProcess == INVALID_HANDLE_VALUE))
            {
                gle = process_notify(*it, transfer_dir, flog); // try to start mergedir.exe
                if(gle)
                {
                    ++it;
                    displayErrorToCerr("watch_notify() process_notify()", gle, &flog);
                }
                else
                {
                    current_notify_file = *it;
                    ha[2] = merge_procinfo.hProcess;
                    notify_merged_list.push_back(*it);
                    it = notify_list.erase(it);
                    transfer_dir.append("\\state");
                    if(opt_verbose) time_header_out(flog) << "mointor " << transfer_dir << endl;
#ifdef _DEBUG
                    time_header_out(cerr) << "mointor " << transfer_dir << endl;
#endif
                    ha[3] = FindFirstChangeNotification(transfer_dir.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
                    if(ha[3] == INVALID_HANDLE_VALUE)
                    {
                        gle = GetLastError();
                        ha[3] = NULL;
                        string msg("watch_notify() FindFirstChangeNotification(");
                        msg.append(transfer_dir).append(")");
                        displayErrorToCerr(msg.c_str(), gle, &flog);
                    }
                    else if(process_merge_notify(false, ha[3], flog)) ha[3] = NULL;
                }
            }
        }
    }
    if(GetSignalInterruptValue() && opt_verbose)
        time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;

clean_child_proc:
    if(hSema)
    {
        while(ReleaseSemaphore(hSema, 1, NULL)) ;
        CloseHandle(hSema);
        hSema = NULL;
    }
    if(ha[3] && ha[3] != INVALID_HANDLE_VALUE) FindCloseChangeNotification(ha[3]);
    if(ha[1] && ha[1] != INVALID_HANDLE_VALUE) FindCloseChangeNotification(ha[1]);
    // ha[0] is qr_procinfo.hProcess
    if(qr_procinfo.hThread && qr_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hThread);
    if(qr_procinfo.hProcess && qr_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hProcess);
    // ha[2] is merge_procinfo.hProcess
    if(merge_procinfo.hThread && merge_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hThread);
    if(merge_procinfo.hProcess && merge_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hProcess);
    return gle;
}
