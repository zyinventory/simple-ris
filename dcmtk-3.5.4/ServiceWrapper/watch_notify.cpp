#include "stdafx.h"
#include "commonlib.h"

using namespace std;

char exec_cmd[1024];

static char buff[1024];
static PROCESS_INFORMATION qr_procinfo, merge_procinfo;
static list<string> notify_list, notify_merged_list;

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
        if(opt_verbose || en != ENOENT)time_header_out(flog) << "find_files() " << filter << " failed: " << buff << endl;
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
static bool process_notify(const string &notify_file, ostream &flog)
{
    string ntffn("store_notify\\");
    ntffn.append(notify_file);
    cerr << notify_file << endl;

    ifstream ntff(ntffn);
    if(ntff.fail()) return false;
    string cmd, path, assoc_id, calling, called, remote, port;
    DWORD tag, pid;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port;
    if(ntff.is_open()) ntff.close();

    if(opt_verbose) cerr << cmd << " " << hex << tag << " " << path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;

    if(path[1] != ':') path.insert(0, "\\pacs\\").insert(0, GetPacsBase());
    string::size_type path_pos = path.find_last_of('\\');
    string association_id = path.substr(path_pos + 1);
    path.erase(path_pos);
    
#ifdef _DEBUG
    strcpy_s(exec_cmd, __FILE__);
    char *p = strrchr(exec_cmd, '\\');
    if(p) *p = '\0'; // cd ..
    p = strrchr(exec_cmd, '\\');
    if(p) *p = '\0'; // cd ..
    if(p) sprintf_s(p, sizeof(exec_cmd) - (p - exec_cmd), "\\Debug\\mergedir.exe %s", association_id.c_str());
    else sprintf_s(exec_cmd, "..\\bin\\mergedir.exe %s", association_id.c_str());
#else
    sprintf_s(exec_cmd, "..\\bin\\mergedir.exe %s", association_id.c_str());
#endif
    DWORD gle = 0;
    gle = create_child_proc(exec_cmd, "mergedir", path.c_str(), &merge_procinfo, flog);
    return true;
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
    WIN32_FIND_DATA wfd;
    sprintf_s(buff, "%s\\pacs\\store_notify", GetPacsBase());
    HANDLE hDirNotify = FindFirstChangeNotification(buff, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hDirNotify == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() FindFirstChangeNotification()", gle, &flog);
        goto clean_child_proc;
    }

    // collect exist dfc files
    find_files(NOTIFY_FILTER, notify_list, notify_merged_list, flog);

    HANDLE ha[3] = { qr_procinfo.hProcess, hDirNotify, NULL };
    size_t hcnt = sizeof(ha) / sizeof(HANDLE);
    while(GetSignalInterruptValue() == 0)
    {
        DWORD wr = 0;
        if(merge_procinfo.hProcess == 0 || merge_procinfo.hProcess == INVALID_HANDLE_VALUE)
            wr = WaitForMultipleObjects(hcnt - 1, ha, FALSE, 1000);
        else
        {
            ha[2] = merge_procinfo.hProcess;
            wr = WaitForMultipleObjects(hcnt, ha, FALSE, 1000);
        }
        if(wr == WAIT_TIMEOUT)
        {
        }
        // todo: else if(wr == WAIT_OBJECT_0) restart dcmqrscp
        else if(wr == WAIT_OBJECT_0 + 1)
        {
            find_files(NOTIFY_FILTER, notify_list, notify_merged_list, flog);
            if(FALSE == FindNextChangeNotification(ha[1]))
            {
                gle = displayErrorToCerr("watch_notify() FindNextChangeNotification()", GetLastError(), &flog);
                goto clean_child_proc;
            }
        }
        else if(wr == WAIT_OBJECT_0 + 2)
        {
            if(opt_verbose) time_header_out(flog) << "watch_notify() mergedir.exe complete" << endl;
            if(merge_procinfo.hThread && merge_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hThread);
            if(merge_procinfo.hProcess && merge_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hProcess);
            memset(&merge_procinfo, 0, sizeof(PROCESS_INFORMATION));
        }
        else
        {
            gle = displayErrorToCerr("watch_notify() WaitForMultipleObjects()", GetLastError(), &flog);
            break;
        }

        if(merge_procinfo.hProcess == 0 || merge_procinfo.hProcess == INVALID_HANDLE_VALUE)
        {
            list<string>::iterator it = notify_list.begin();
            while(it != notify_list.end() &&
                (merge_procinfo.hProcess == 0 || merge_procinfo.hProcess == INVALID_HANDLE_VALUE))
            {
                if(process_notify(*it, flog))
                {
                    notify_merged_list.push_back(*it);
                    it = notify_list.erase(it);
                }
                else
                    ++it;
            }
        }
    }
    if(opt_verbose && GetSignalInterruptValue())
        time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;

clean_child_proc:
    if(hDirNotify && hDirNotify != INVALID_HANDLE_VALUE) FindCloseChangeNotification(hDirNotify);
    if(qr_procinfo.hThread && qr_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hThread);
    if(qr_procinfo.hProcess && qr_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hProcess);
    if(merge_procinfo.hThread && merge_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hThread);
    if(merge_procinfo.hProcess && merge_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(merge_procinfo.hProcess);
    return gle;
}
