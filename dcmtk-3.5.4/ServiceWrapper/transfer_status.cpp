#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;
using namespace handle_context;

const char* handle_context::translate_action_type(ACTION_TYPE t)
{
    switch(t)
    {
    case NO_ACTION: return "NO_ACTION";
    case BURN_PER_STUDY_RELEASE: return "BURN_PER_STUDY_RELEASE";
    case BURN_PER_STUDY_ABORT: return "BURN_PER_STUDY_ABORT";
    case BURN_MULTI: return "BURN_MULTI";
    case INDEX_INSTANCE:  return "INDEX_INSTANCE";
    default: return "ERROR_ACTION_TYPE";
    }
}

const char* _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION::StorePath(char sp)
{
    HashStr(studyUID, unique_filename, sizeof(unique_filename));
    unique_filename[8] = sp;
    SeriesInstancePath(seriesUID, instanceUID, unique_filename + 9, sizeof(unique_filename) - 9, sp);
    sprintf_s(hash, "%c%c%c%c%c%c%c%c%c%c%c",
        unique_filename[0], unique_filename[1], sp, unique_filename[2], unique_filename[3], sp, 
        unique_filename[4], unique_filename[5], sp, unique_filename[6], unique_filename[7]);
    return unique_filename;
}

bool _tag_NOTIFY_FILE_CONTEXT::operator<(const struct _tag_NOTIFY_FILE_CONTEXT &r) const
{
    int cmp = strcmp(src_notify_filename, r.src_notify_filename);
    if(cmp < 0) return true;
    else if(cmp > 0) return false;
    else
    {
        cmp = strcmp(file.unique_filename, r.file.unique_filename);
        if(cmp < 0) return true;
        else if(cmp > 0) return false;
        else
        {
            cmp = strcmp(file.filename, r.file.filename);
            if(cmp < 0) return true;
            else return false;
        }
    }
}

notify_file& notify_file::operator=(const notify_file &r)
{
    handle_waitable::operator=(r);
    association_id = r.association_id;
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
    last_association_notify_filename = r.last_association_notify_filename;
    assoc_disconn = r.assoc_disconn;
    disconn_release = r.disconn_release;
    copy(r.list_file.begin(), r.list_file.end(), back_inserter(list_file));
    copy(r.set_study.begin(), r.set_study.end(), inserter(set_study, set_study.end()));
    copy(r.set_complete.begin(), r.set_complete.end(), inserter(set_complete, set_complete.end()));
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
    new_files.sort();
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

handle_context::NOTIFY_FILE_CONTEXT* handle_dir::process_notify_file(std::istream &ifs, unsigned int seq, ostream &flog)
{
    NOTIFY_FILE_CONTEXT *pclc = new NOTIFY_FILE_CONTEXT;
    memset(pclc, 0, sizeof(NOTIFY_FILE_CONTEXT));

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
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " "
            << callingAE << " " << callingAddr<< " " << calledAE<< " " << dec << port << " " << expected_xfer << " "<< calledAddr << endl;
        break;
    case NOTIFY_ASSOC_RELEASE:
        assoc_disconn = true;
        disconn_release = true;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " disconnect release" << endl;
        break;
    case NOTIFY_ASSOC_ABORT:
        assoc_disconn = true;
        disconn_release = false;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " disconnect abort" << endl;
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
        NOTIFY_FILE_CONTEXT *pnfc = process_notify_file(ifs, tag, flog);
        if(pnfc)
        {
            pnfc->handle_dir_ptr = this;
            strcpy_s(pnfc->src_notify_filename, filename.c_str());
            pnfc->file.StorePath();
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify(" << filepath << ") " << get_association_id() << " read OK." << endl;
#ifdef _DEBUG
            time_header_out(cerr) << "handle_dir::process_notify(" << filepath << ") " << get_association_id() << " read OK." << endl;
#endif
            // don't insert file to set_complete now, insert it after compress complete
            compress_queue.push_back(*pnfc);

            string study_uid(pnfc->file.studyUID);
            this->insert_study(study_uid); // association[1] -> study[n]
            
            handle_dicomdir* phd = named_pipe_server::named_pipe_server_ptr->make_handle_dicomdir(study_uid);
            if(phd) phd->insert_association_path(get_path());  // study[1] -> association[n]

            delete pnfc;
        }
        else
        {
            time_header_out(flog) << "handle_dir::process_notify() can't process " << filename << ", ignore." << endl;
            set_complete.insert(filename);
        }
    }
    else if(cmd.compare(NOTIFY_STORE_TAG) == 0)
    {
        process_notify_association(ifs, tag, flog);
        last_association_notify_filename = filename;
        set_complete.insert(filename);
    }
    else if(cmd.compare(NOTIFY_ACKN_ITEM) == 0)
    {
        if(tag == NOTIFY_COMPRESS_OK)
        {
            string src_notify_file;
            ifs >> src_notify_file;
            set_complete.insert(src_notify_file);
            set_complete.insert(filename); // complete ack self
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify() recieve compress complete notify " << filename << "(" << src_notify_file << ")." << endl;
        }
        else if(tag == NOTIFY_ALL_COMPRESS_OK)
            set_complete.insert(filename);
        else
            time_header_out(flog) << "handle_dir::process_notify() ignore ack file " << filename << endl
                << cmd << " " << hex << uppercase << tag << " ..." << endl;
    }
    else
        time_header_out(flog) << "handle_dir::process_notify() ignore " << filename << endl;

    if(ifs.is_open()) ifs.close();
    return 0;
}

void handle_dir::send_compress_complete_notify(const NOTIFY_FILE_CONTEXT &nfc, handle_dicomdir *phdir, ostream &flog)
{
    if(phdir) phdir->append_action(action_from_association(nfc));

    char notify_file_name[MAX_PATH];
    string prefix(get_path());
    prefix.append("\\state\\");
    size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), prefix.c_str());
    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out, _SH_DENYRW);
    if(ntf.good())
    {
        save_notify_context_to_ostream(nfc, ntf);
        ntf.close();
        if(opt_verbose) time_header_out(flog) << "handle_dir::send_compress_complete_notify() to " << notify_file_name << " OK." << endl;
    }
    else
    {
        time_header_out(flog) << "handle_dir::send_compress_complete_notify() to " << notify_file_name << " failed:" << endl;
        save_notify_context_to_ostream(nfc, flog);
    }
}

void handle_dir::check_complete_remain(std::ostream &flog) const
{
    set<string> remain(set_complete);
    time_header_out(flog) << "association " << get_association_id() << " " << get_path() << " file " << dec << list_file.size() << ", complete " << set_complete.size() << endl;
    for(list<string>::const_iterator it = list_file.begin(); it != list_file.end(); ++it)
    {
        if(remain.find(*it) == remain.end())
            flog << "\t" << *it << " not complete" << endl;
        else
            remain.erase(*it);
    }
    for(set<string>::iterator it = remain.begin(); it != remain.end(); ++it)
        flog << "\t" << *it << " unexcepted complete" << endl;
    for(set<string>::iterator it = set_study.begin(); it != set_study.end(); ++it)
        flog << "\tstudy " << *it << endl;
}

void handle_dir::send_all_compress_ok_notify_and_close_handle(ostream &flog)
{
    // close motinor handle, avoid all_compress_ok notify loop
    FindCloseChangeNotification(handle);
    handle = NULL;

    char notify_file_name[MAX_PATH];
    string prefix(get_path());
    prefix.append("\\state\\");
    size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), prefix.c_str());
    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out, _SH_DENYRW);
    if(ntf.good())
    {
        ntf << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << " " << last_association_notify_filename << endl;
        ntf.close();
        if(opt_verbose) time_header_out(flog) << "watch_notify() send all compress OK notify " << notify_file_name << " OK." << endl;
    }
    else
    {
        time_header_out(flog) << "watch_notify() send all compress OK notify " << notify_file_name << " failed:" << endl;
        flog << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;
    }
}

void handle_dir::broadcast_action_to_all_study(named_pipe_server &nps, ostream &flog) const
{
    for(set<string>::iterator it = set_study.begin(); it != set_study.end(); ++it)
    {
        handle_dicomdir *phd = nps.find_handle_dicomdir(*it);
        if(phd)
        {
            if(assoc_disconn)
            {
                action_from_association *paaa = NULL;
                paaa = new action_from_association((disconn_release ? ACTION_TYPE::BURN_PER_STUDY_RELEASE : ACTION_TYPE::BURN_PER_STUDY_ABORT));
                phd->append_action_and_erease_association(*paaa, get_association_id(), get_path(), flog);
                delete paaa;
            }
            else time_header_out(flog) << "handle_dir::broadcast_action_to_all_study() " << *it << " : association is not disconnected." << endl;
        }
        else time_header_out(flog) << "handle_dir::broadcast_action_to_all_study() can't find study " << *it << endl;
    }
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

handle_compress* handle_compress::make_handle_compress(const NOTIFY_FILE_CONTEXT &nfc)
{
    const char *verbose_flag = opt_verbose ? "-v" : "";
#ifdef _DEBUG
    int mkdir_pos = 0;
    char cmd[1024] = __FILE__;
    char *p = strrchr(cmd, '\\');
    if(p)
    {
        ++p;
        mkdir_pos = p - cmd;
        mkdir_pos += sprintf_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", verbose_flag, nfc.file.filename);
    }
    else
        mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", COMMONLIB_PACS_BASE, verbose_flag, nfc.file.filename);
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", COMMONLIB_PACS_BASE, verbose_flag, nfc.file.filename);
#endif
    int ctn = mkdir_pos;
    ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "..\\..\\archdir\\v0000000\\%s\\%s\\", nfc.file.hash, nfc.file.studyUID);
    strcpy_s(cmd + ctn, sizeof(cmd) - ctn, nfc.file.unique_filename);

    return new handle_compress(nfc.handle_dir_ptr, cmd, "dcmcjpeg", nfc);
}

handle_compress& handle_compress::operator=(const handle_compress &r)
{
    handle_proc::operator=(r);
    notify_ctx = r.notify_ctx;
    return *this;
}

handle_dicomdir& handle_dicomdir::operator=(const handle_dicomdir &r)
{
    handle_proc::operator=(r);
    study_uid = r.study_uid;
    dicomdir_path = r.dicomdir_path;
    copy(r.set_association_path.begin(), r.set_association_path.end(), inserter(set_association_path, set_association_path.end()));
    copy(r.list_action.begin(), r.list_action.end(), back_inserter(list_action));
    return *this;
}

void handle_dicomdir::append_action_and_erease_association(const action_from_association &action, const string &assoc_id, const string &assoc_path, ostream &flog)
{
    set_association_path.erase(assoc_path);
    if(opt_verbose) time_header_out(flog) << "handle_dicomdir::append_action_and_erease_association() " << study_uid << " add action " << translate_action_type(action.type) << endl
        << "\tand erease assoc " << assoc_id << " " << assoc_path << endl;
    list_action.push_back(action);
}

void handle_dicomdir::print_state(ostream &flog) const
{
    time_header_out(flog) << "study " << study_uid << endl;
    // print associations
    for(set<string>::const_iterator it = set_association_path.begin(); it != set_association_path.end(); ++it)
        flog << "\tassociation path " << *it << endl;
    // print actions
    flog << "actions:" << endl;
    for(list<action_from_association>::const_iterator it = list_action.begin(); it != list_action.end(); ++it)
    {
        flog << "\t" << translate_action_type(it->type);
        if(it->type == ACTION_TYPE::INDEX_INSTANCE)
        {
            if(it->pnfc) flog << " " << it->pnfc->file.unique_filename;
            else flog << " pnfc is NULL";
        }
        else if(it->type == ACTION_TYPE::BURN_MULTI) flog << it->burn_multi_id;
        flog << endl;
    }
}

action_from_association& action_from_association::operator=(const action_from_association &r)
{
    type = r.type;
    burn_multi_id = r.burn_multi_id;
    if(r.pnfc)
    {
        pnfc = new NOTIFY_FILE_CONTEXT;
        *pnfc = *r.pnfc;
    }
    else pnfc = r.pnfc; // r.pnfc is NULL
    return *this;
}

bool action_from_association::operator<(const action_from_association &r) const
{
    if(type < r.type) return true;
    else if(r.type < type) return false;
    else // type == r.type
    {
        if(pnfc == r.pnfc) return false;
        else if(pnfc == NULL) return true; // r.pnfc != NULL
        else if(r.pnfc == NULL) return false; // pnfc != NULL
        else // all is not NULL
        {
            if(*pnfc < *r.pnfc) return true;
            else if(*r.pnfc < *pnfc) return false;
            else return burn_multi_id < r.burn_multi_id; // ==
        }
    }
}
