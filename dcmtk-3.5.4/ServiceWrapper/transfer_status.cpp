#include "stdafx.h"
#include "commonlib.h"

using namespace std;
using namespace handle_context;

notify_file& notify_file::operator=(const notify_file &r)
{
    association_id = r.association_id;
    path = r.path;
    return *this;
}

handle_dir& handle_dir::operator=(const handle_dir &r)
{
    notify_file::operator=(r);
    handle = r.handle;
    copy(r.filelist.begin(), r.filelist.end(), filelist.begin());
    return *this;
}

string& handle_dir::get_find_filter(std::string &filter) const
{
    filter = get_path();
    if(get_association_id().length()) filter.append("\\state"); // is association
    // else is store_notify
    return filter.append("\\*.dfc");
}

DWORD handle_dir::find_files(std::ostream &flog, std::function<DWORD(const std::string&)> pred)
{
    struct _finddata_t wfd;
    string filter;
    intptr_t hSearch = _findfirst(get_find_filter(filter).c_str(), &wfd);
    if(hSearch == -1)
    {
        errno_t en = errno;
        if(en == ENOENT) return 0;
        else
        {
            char buff[1024];
            strerror_s(buff, en);
            if(opt_verbose) time_header_out(flog) << "handle_dir::find_files() " << filter << " failed: " << buff << endl;
            return -1;
        }
    }

    bool process_file_error = false;
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0) 
			continue; // skip . .. DICOMDIR
        if((wfd.attrib & _A_SUBDIR) == 0)
        {
            if(find(filelist.begin(), filelist.end(), node) == filelist.end())
            {
                if(pred(node))
                {
                    process_file_error = true;
                    break;
                }
                else filelist.push_back(node);
            }
        }
	} while(_findnext(hSearch, &wfd) == 0);
    last_find_error = process_file_error;
	_findclose(hSearch);
    filelist.sort();

    if(FALSE == FindNextChangeNotification(handle))
        return displayErrorToCerr("handle_dir::find_files() FindNextChangeNotification()", GetLastError(), &flog);
    return 0;
}

DWORD handle_dir::process_notify_file(const std::string &filename, std::ostream &flog)
{
    string filepath(get_path());
    filepath.append(1, '\\').append(filename);
    cerr << "handle_dir::process_notify_file(" << filepath << ")" << endl;
    return 0;
}

handle_proc& handle_proc::operator=(const handle_proc &r)
{
    notify_file::operator=(r);
    exec_cmd = r.exec_cmd;
    procinfo = r.procinfo;
    return *this;
}

int handle_proc::create_process(const char *exec_name, std::ostream &flog)
{
    STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);
    memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));

	SECURITY_ATTRIBUTES logSA;
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE logFile = INVALID_HANDLE_VALUE;
    char buff[1024];
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
    strcpy_s(buff, exec_cmd.c_str());
    if( CreateProcess(NULL, buff, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, get_path().c_str(), &sinfo, &procinfo) )
	{
        CloseHandle(logFile);
        if(opt_verbose) time_header_out(flog) << "create_child_proc(" << exec_cmd << ") OK" << endl;
		return 0;
	}
	else
	{
        DWORD gle = GetLastError();
        CloseHandle(logFile);
		return gle;
	}
}

bool read_notify_info(const string filename)
{

    return true;
}
