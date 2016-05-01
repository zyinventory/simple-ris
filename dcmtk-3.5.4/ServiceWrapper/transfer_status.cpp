#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;
using namespace handle_context;

const char* CMOVE_FILE_SECTION::StorePath(char sp)
{
    HashStr(studyUID, unique_filename, sizeof(unique_filename));
    unique_filename[8] = sp;
    SeriesInstancePath(seriesUID, instanceUID, unique_filename + 9, sizeof(unique_filename) - 9, sp);
    sprintf_s(hash, "%c%c%c%c%c%c%c%c%c%c%c",
        unique_filename[0], unique_filename[1], sp, unique_filename[2], unique_filename[3], sp, 
        unique_filename[4], unique_filename[5], sp, unique_filename[6], unique_filename[7]);
    return unique_filename;
}

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
    store_assoc_id = r.store_assoc_id;
    callingAE = r.callingAE;
    callingAddr = r.calledAddr;
    calledAE = r.calledAE;
    calledAddr = r.calledAddr;
    expected_xfer = r.expected_xfer;
    port = r.port;
    assoc_disconn = r.assoc_disconn;
    disconn_release = r.disconn_release;
    copy(r.list_file.begin(), r.list_file.end(), back_inserter(list_file));
    copy(r.set_study.begin(), r.set_study.end(), inserter(set_study, set_study.end()));
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
    list<string> new_files;
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0) 
			continue; // skip . .. DICOMDIR
        if((wfd.attrib & _A_SUBDIR) == 0)
        {
            if(find(list_file.begin(), list_file.end(), node) == list_file.end())
                new_files.push_back(node);
        }
	} while(_findnext(hSearch, &wfd) == 0);
    last_find_error = process_file_error;
	_findclose(hSearch);

    if(FALSE == FindNextChangeNotification(handle))
        return displayErrorToCerr("handle_dir::find_files() FindNextChangeNotification()", GetLastError(), &flog);

    // handle OK, process new files.
    for(list<string>::iterator it = new_files.begin(); it != new_files.end(); ++it)
    {
        DWORD gle = pred(*it);
        if(gle && gle == ERROR_SHARING_VIOLATION)
        {
            process_file_error = true;
            break; // try it later
        }
        else list_file.push_back(*it); // success or other error, no more process it
    }
    list_file.sort();
    return 0;
}

handle_context::CMOVE_NOTIFY_CONTEXT* handle_dir::process_notify_file(std::istream &ifs, unsigned int seq, ostream &flog)
{
    CMOVE_NOTIFY_CONTEXT *pclc = new CMOVE_NOTIFY_CONTEXT;
    memset(pclc, 0, sizeof(CMOVE_NOTIFY_CONTEXT));

    if(seq >= NOTIFY_FILE_SEQ_START)
    {
        pclc->file_seq = seq;
        ifs >> pclc->file.filename;
        if(opt_verbose) time_header_out(flog) << NOTIFY_FILE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << seq 
            << " " << pclc->file.filename << endl;
    }
    else
    {
        time_header_out(flog) << "handle_dir::process_notify_file() seq is error: " 
            << NOTIFY_FILE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << seq 
            << " " << pclc->file.filename << endl;
        return NULL;
    }

    char *cmd = new char[1024];
    do{
        ifs.getline(cmd, 1024);
        if(ifs.gcount() && strlen(cmd))
        {
            istringstream linestrm(cmd);
            string type;
            linestrm >> type;
            if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_INSTANCE))
                cmd_instance(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_PATIENT))
                cmd_patient(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_STUDY))
                cmd_study(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_LEVEL_SERIES))
                cmd_series(type, linestrm, *pclc, flog);
            else if(STRING_PRE4_TO_INT(type) == CHAR4_TO_INT(NOTIFY_FILE_TAG))
                ; // ignore it
            else 
                time_header_out(flog) << "handle_dir::process_notify_file() can't recognize line: " << linestrm.str() << endl;
        }
    } while(ifs.good());
    if(cmd) delete[] cmd;
    return pclc;
}

void handle_dir::process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog)
{
    switch(tag)
    {
    case NOTIFY_ASSOC_ESTA:
        ifs >> store_assoc_id >> callingAE >> callingAddr >> calledAE >> dec >> port >> expected_xfer >> calledAddr;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << store_assoc_id << " "
            << callingAE << " " << callingAddr<< " " << calledAE<< " " << dec << port << " " << expected_xfer << " "<< calledAddr << endl;
        break;
    case NOTIFY_ASSOC_RELEASE:
        assoc_disconn = true;
        disconn_release = true;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << store_assoc_id << " disconnect release" << endl;
        break;
    case NOTIFY_ASSOC_ABORT:
        assoc_disconn = true;
        disconn_release = false;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << store_assoc_id << " disconnect abort" << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            ifs.getline(otherbuf, sizeof(otherbuf));
            if(!opt_verbose) time_header_out(flog) << NOTIFY_STORE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << tag << " "
                << " " << otherbuf << ", encounter error" << endl;
        }
        break;
    }
}

DWORD handle_dir::process_notify(const std::string &filename, std::ostream &flog)
{
    DWORD gle = 0, tag;
    string cmd, filepath(get_path());
    filepath.append("\\state\\").append(filename);
    if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_file(" << filepath << ") " << get_association_id() << endl;
    ifstream ifs(filepath, ios_base::in, _SH_DENYRW);
    if(ifs.fail())
    {
        gle = GetLastError();
        string msg("handle_dir::process_notify_file() open file ");
        msg.append(filepath);
        return displayErrorToCerr(msg.c_str(), gle, &flog);
    }

    ifs >> cmd >> hex >> tag;
    
    if(cmd.compare(NOTIFY_FILE_TAG) == 0) // receive a file
    {
        CMOVE_NOTIFY_CONTEXT *pclc = process_notify_file(ifs, tag, flog);
        if(pclc)
        {
            strcpy_s(pclc->association_id, get_association_id().c_str());
            pclc->file.StorePath();
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify(" << filepath << ") " << get_association_id() << " read OK" << endl;
#ifdef _DEBUG
            time_header_out(cerr) << "handle_dir::process_notify(" << filepath << ") " << get_association_id() << " read OK" << endl;
#endif
            compress_queue.push_back(*pclc);
            string study_uid(pclc->file.studyUID);
            this->insert_study(study_uid); // association[1] -> study[n]
            
            handle_dicomdir* phd = NULL;
            STUDY_MAP::iterator it = map_dicomdir.find(study_uid);
            if(it == map_dicomdir.end())
            {
                phd = handle_dicomdir::make_handle_dicomdir(study_uid);
                if(phd) map_dicomdir[study_uid] = phd;
            }
            else
                phd = it->second;
            
            if(phd) phd->insert_association_path(get_path());  // study[1] -> association[n]

            delete pclc;
        }
    }
    else if(cmd.compare(NOTIFY_STORE_TAG) == 0)
    {
        process_notify_association(ifs, tag, flog);
    }
    // else ignore
    if(ifs.is_open()) ifs.close();
    return 0;
}

handle_proc& handle_proc::operator=(const handle_proc &r)
{
    notify_file::operator=(r);
    hlog = r.hlog;
    exec_cmd = r.exec_cmd;
    exec_name = r.exec_name;
    log_path = r.log_path;
    procinfo = r.procinfo;
    return *this;
}

handle_proc::~handle_proc()
{
    if(procinfo.hThread && procinfo.hThread != INVALID_HANDLE_VALUE) CloseHandle(procinfo.hThread);
    if(procinfo.hProcess && procinfo.hProcess != INVALID_HANDLE_VALUE) CloseHandle(procinfo.hProcess);
    if(hlog && hlog != INVALID_HANDLE_VALUE)
    {
        DWORD file_size = GetFileSize(hlog, NULL);
        CloseHandle(hlog);
        if(file_size == 0)
        {
            if(_unlink(log_path.c_str()))
            {
                char msg[1024];
                strerror_s(msg, errno);
                time_header_out(cerr) << "handle_proc::~handle_proc() _unlink(" << log_path << ") failed: " << msg << endl;
            }
        }
    }
}

#define START_PROCESS_BUFF_SIZE 1024

int handle_proc::start_process(std::ostream &flog)
{
    STARTUPINFO sinfo;
	memset(&sinfo, 0, sizeof(STARTUPINFO));
	sinfo.cb = sizeof(STARTUPINFO);

    if(procinfo.hThread) CloseHandle(procinfo.hThread);
    if(procinfo.hProcess) CloseHandle(procinfo.hProcess);
    memset(&procinfo, 0, sizeof(PROCESS_INFORMATION));

	SECURITY_ATTRIBUTES logSA;
	logSA.bInheritHandle = TRUE;
	logSA.lpSecurityDescriptor = NULL;
	logSA.nLength = sizeof(SECURITY_ATTRIBUTES);

	HANDLE logFile = INVALID_HANDLE_VALUE;
    char buff[START_PROCESS_BUFF_SIZE];
	size_t pos = GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_", buff, START_PROCESS_BUFF_SIZE);
    sprintf_s(buff + pos, START_PROCESS_BUFF_SIZE - pos, "%s_%x.txt", exec_name.c_str(), this);

	if(PrepareFileDir(buff))
		logFile = CreateFile(buff, GENERIC_WRITE, FILE_SHARE_READ, &logSA, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if(logFile != INVALID_HANDLE_VALUE)
	{
		sinfo.dwFlags |= STARTF_USESTDHANDLES;
		sinfo.hStdOutput = logFile;
		sinfo.hStdError = logFile;
		sinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        log_path = buff;
	}
    strcpy_s(buff, START_PROCESS_BUFF_SIZE, exec_cmd.c_str());
    if( CreateProcess(NULL, buff, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP, NULL, get_path().c_str(), &sinfo, &procinfo) )
	{
        if(FALSE == DuplicateHandle(GetCurrentProcess(), logFile, GetCurrentProcess(), &hlog, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_CLOSE_SOURCE))
            hlog = NULL;
        if(opt_verbose) time_header_out(flog) << "handle_proc::start_process(" << exec_cmd << ") OK" << endl;
		return 0;
	}
	else
    {
        CloseHandle(logFile);
        return GetLastError();
    }
}

handle_compress* handle_compress::make_handle_compress(const CMOVE_NOTIFY_CONTEXT &cnc, HANDLE_MAP &map_handle)
{
    string assoc_id(cnc.association_id);
    // select cwd form assoc_id
    const HANDLE_MAP::iterator it = find_if(map_handle.begin(), map_handle.end(),
        [&assoc_id](const HANDLE_PAIR &p) -> bool {
            if(0 == assoc_id.compare(p.second->get_association_id()))
                return (NULL != dynamic_cast<handle_dir*>(p.second));
            else
                return false;
        });
    if(it == map_handle.end()) return NULL;
    
    const char *verbose_flag = opt_verbose ? "-v" : "";
#ifdef _DEBUG
    int mkdir_pos = 0;
    char cmd[1024] = __FILE__;
    char *p = strrchr(cmd, '\\');
    if(p)
    {
        ++p;
        mkdir_pos = p - cmd;
        mkdir_pos += sprintf_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", verbose_flag, cnc.file.filename);
    }
    else
        mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", COMMONLIB_PACS_BASE, verbose_flag, cnc.file.filename);
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", COMMONLIB_PACS_BASE, verbose_flag, cnc.file.filename);
#endif
    int ctn = mkdir_pos;
    ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "archdir\\%s\\", cnc.file.studyUID);
    strcpy_s(cmd + ctn, sizeof(cmd) - ctn, cnc.file.unique_filename);

    return new handle_compress(assoc_id, it->second->get_path(), cmd, "dcmcjpeg", cnc);
}

handle_compress& handle_compress::operator=(const handle_compress &r)
{
    handle_proc::operator=(r);
    notify_ctx = r.notify_ctx;
    return *this;
}

handle_dicomdir* handle_dicomdir::make_handle_dicomdir(const std::string &study)
{
    char dicomdir[1024], hash[9];
    string pacs_dir(GetPacsBase());
    pacs_dir.append("\\pacs");
    HashStr(study.c_str(), hash, sizeof(hash));
    sprintf_s(dicomdir, "%s\\archdir\\v0000000\\%c%c\\%c%c\\%c%c\\%c%c\\%s.dir", pacs_dir.c_str(),
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], study.c_str());
    PrepareFileDir(dicomdir);

#ifdef _DEBUG
    int mkdir_pos = 0;
    char cmd[1024] = __FILE__;
    char *p = strrchr(cmd, '\\');
    if(p)
    {
        ++p;
        strcpy_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmmkdir.exe --general-purpose-dvd -A ");
        mkdir_pos = strlen(cmd);
    }
    else
        mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", GetPacsBase());
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", GetPacsBase());
#endif
    sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s +id . +D %s --viewer GE -pn %s #", 
        opt_verbose ? "-v" : "", dicomdir, study.c_str());

    return new handle_dicomdir("", pacs_dir, cmd, "dcmmkdir", dicomdir, study);
}

handle_dicomdir& handle_dicomdir::operator=(const handle_dicomdir &r)
{
    handle_proc::operator=(r);
    study_uid = r.study_uid;
    dicomdir_path = r.dicomdir_path;
    copy(r.set_association_path.begin(), r.set_association_path.end(), inserter(set_association_path, set_association_path.end()));
    return *this;
}
