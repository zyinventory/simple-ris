#include "stdafx.h"
#include "commonlib.h"

using namespace std;

static char buff[1024];
static PROCESS_INFORMATION qr_procinfo;

char qrcmd[1024];

static int create_dcmqrscp(ostream &flog)
{
	STARTUPINFO sinfo;
	memset(&qr_procinfo, 0, sizeof(PROCESS_INFORMATION));
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

	SECURITY_ATTRIBUTES logSA;
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE logFile = INVALID_HANDLE_VALUE;
	GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_dcmqrscp.txt", buff, sizeof(buff));
	if(PrepareFileDir(buff))
		logFile = CreateFile(buff, GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if(logFile != INVALID_HANDLE_VALUE)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdOutput = logFile;
		sinfo.hStdError = logFile;
		sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	}

	if( CreateProcess(NULL, qrcmd, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, NULL, &sinfo, &qr_procinfo) )
	{
        CloseHandle(logFile);
        if(opt_verbose) time_header_out(flog) << "create_dcmqrscp() OK: " << qrcmd << endl;
		return 0;
	}
	else
	{
        DWORD gle = GetLastError();
        CloseHandle(logFile);
		return gle;
	}
}

static int find_files(const char *filter, list<string> &filelist, ostream &flog)
{
    struct _finddata_t wfd;
    intptr_t hSearch = _findfirst(filter, &wfd);
    if(hSearch == -1)
    {
        strerror_s(buff, errno);
        time_header_out(flog) << "find_files() " << filter << " failed: " << buff << endl;
        return -1;
    }
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0) 
			continue; // skip . .. DICOMDIR
        if((wfd.attrib & _A_SUBDIR) == 0)
        {
            if(find(filelist.begin(), filelist.end(), node) == filelist.end())
                filelist.push_back(node);
        }
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);
    return filelist.size();
}

// if notify compress and index OK, return true, erease it;
// else remain it.
static bool process_notify(const string &notify_file)
{
    cout << notify_file << endl;
    return true;
}

#define NOTIFY_FILTER "store_notify\\*.dfc"

int watch_notify(ostream &flog)
{
    DWORD gle = create_dcmqrscp(flog);
    if(gle)
    {
        displayErrorToCerr("watch_notify() create_dcmqrscp() at beginning", gle, &flog);
        return gle;
    }
#ifdef _DEBUG
    WaitForInputIdle(qr_procinfo.hProcess, INFINITE);
#endif
    list<string> notify_list;
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
    find_files(NOTIFY_FILTER, notify_list, flog);

    HANDLE ha[2] = { hDirNotify, qr_procinfo.hProcess };
    DWORD wr = 0;
    while(wr = WaitForMultipleObjects(2, ha, FALSE, 1000))
    {
        if(wr == WAIT_TIMEOUT)
        {
            if(GetSignalInterruptValue())
            {
                if(opt_verbose) time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;
                break;
            }
        }
        else if(wr == WAIT_OBJECT_0)
        {
            find_files(NOTIFY_FILTER, notify_list, flog);
        }
        else
        {
            gle = displayErrorToCerr("watch_notify() WaitForMultipleObjects()", GetLastError(), &flog);
            break;
        }

        list<string>::iterator it = notify_list.begin();
        while(it != notify_list.end())
        {
            if(process_notify(*it))
                it = notify_list.erase(it);
            else
                ++it;
        }
    }

clean_child_proc:
    if(hDirNotify && hDirNotify != INVALID_HANDLE_VALUE) FindCloseChangeNotification(hDirNotify);
    if(qr_procinfo.hThread && qr_procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hThread);
    if(qr_procinfo.hProcess && qr_procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(qr_procinfo.hProcess);
    return gle;
}
