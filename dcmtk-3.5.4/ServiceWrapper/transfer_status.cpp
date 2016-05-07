#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;
using namespace handle_context;

const char* action_from_association::translate_action_type(ACTION_TYPE t)
{
    switch(t)
    {
    case NO_ACTION: return "NO_ACTION";
    case BURN_PER_STUDY: return "BURN_PER_STUDY";
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

void base_path::print_state() const
{
    *pflog << "base_path::print_state() path: " << path << endl;
}

void meta_notify_file::print_state() const
{
    *pflog << "meta_notify_file::print_state() association_id: " << association_id << endl
        << "\tmeta_notify_filename: " << meta_notify_filename << endl;
    base_path::print_state();
}

meta_notify_file& meta_notify_file::operator=(const meta_notify_file &r)
{
    base_path::operator=(r);
    association_id = r.association_id;
    meta_notify_filename = r.meta_notify_filename;
    return *this;
}

handle_dir& handle_dir::operator=(const handle_dir &r)
{
    meta_notify_file::operator=(r);
    handle = r.handle;
    store_assoc_id = r.store_assoc_id;
    callingAE = r.callingAE;
    callingAddr = r.calledAddr;
    calledAE = r.calledAE;
    calledAddr = r.calledAddr;
    expected_xfer = r.expected_xfer;
    auto_publish = r.auto_publish;
    port = r.port;
    last_association_notify_filename = r.last_association_notify_filename;
    assoc_disconn = r.assoc_disconn;
    disconn_release = r.disconn_release;
    last_find_error = r.last_find_error;
    copy(r.list_file.begin(), r.list_file.end(), back_inserter(list_file));
    copy(r.set_study.begin(), r.set_study.end(), inserter(set_study, set_study.end()));
    copy(r.set_complete.begin(), r.set_complete.end(), inserter(set_complete, set_complete.end()));
    return *this;
}

handle_dir::~handle_dir()
{
    if(handle) FindCloseChangeNotification(handle);

    string meta_file(get_meta_notify_filename());
    if(meta_file.length())
    {
        meta_file.insert(0, 1, '\\').insert(0, NOTIFY_BASE);
        if(_unlink(meta_file.c_str()))
        {
            char msg[1024];
            strerror_s(msg, errno);
            time_header_out(*pflog) << __FUNCSIG__" _unlink(" << meta_file << ") failed: " << msg << endl;
        }
    }
}

void handle_dir::print_state() const
{
    *pflog << "handle_dir::print_state()" << endl
        << "\tstore_assoc_id: " << store_assoc_id << endl
        << "\tcallingAE: " << callingAE << endl
        << "\tcallingAddr: " << callingAddr << endl
        << "\tcalledAE: " << calledAE << endl
        << "\tcalledAddr: " << calledAddr << endl
        << "\texpected_xfer: " << expected_xfer << endl
        << "\tauto_publish: " << auto_publish << endl
        << "\tport: " << dec << port << endl
        << "\tlast_association_notify_filename: " << last_association_notify_filename << endl
        << "\tassoc_disconn: " << assoc_disconn << endl
        << "\tdisconn_release: " << disconn_release << endl
        << "\tlast_find_error: " << last_find_error << endl;
    set<string> remain(set_complete);
    *pflog << "\tassociation " << get_association_id() << " " << get_path() << " file " << dec << list_file.size() << ", complete " << set_complete.size() << endl;
    for(list<string>::const_iterator it = list_file.begin(); it != list_file.end(); ++it)
    {
        if(remain.find(*it) == remain.end())
            *pflog << "\t" << *it << " not complete" << endl;
        else
            remain.erase(*it);
    }
    for(set<string>::iterator it = remain.begin(); it != remain.end(); ++it)
        *pflog << "\t" << *it << " unexcepted complete" << endl;
    for(set<string>::iterator it = set_study.begin(); it != set_study.end(); ++it)
        *pflog << "\tstudy " << *it << endl;

    meta_notify_file::print_state();
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
        ifs >> store_assoc_id >> callingAE >> callingAddr >> calledAE >> dec >> port >> expected_xfer >> auto_publish >> calledAddr;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " "
            << callingAE << " " << callingAddr<< " " << calledAE<< " " << dec << port << " " << expected_xfer << " "<< calledAddr << endl;
        break;
    case NOTIFY_ASSOC_RELEASE:
        assoc_disconn = true;
        disconn_release = true;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " disconnect release." << endl;
        break;
    case NOTIFY_ASSOC_ABORT:
        assoc_disconn = true;
        disconn_release = false;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_association_id() << " disconnect abort." << endl;
        break;
    default:
        {
            char otherbuf[1024] = "";
            ifs.getline(otherbuf, sizeof(otherbuf));
            if(!opt_verbose) time_header_out(flog) << NOTIFY_STORE_TAG << " " << hex << uppercase << setw(8) << setfill('0') << tag << " "
                << " " << otherbuf << ", encounter error." << endl;
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
            
            handle_study* phs = named_pipe_server::get_named_pipe_server_singleton()->make_handle_study(study_uid);
            if(phs) phs->insert_association_path(get_path());  // add association lock to study, study[1] -> association[n]

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

void handle_dir::send_compress_complete_notify(const NOTIFY_FILE_CONTEXT &nfc, handle_study *phs, ostream &flog)
{
    if(phs) phs->append_action(action_from_association(nfc, get_path(), &flog));

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

void handle_dir::broadcast_action_to_all_study(named_pipe_server &nps) const
{
    for(set<string>::iterator it = set_study.begin(); it != set_study.end(); ++it)
    {
        handle_study *phs = nps.find_handle_study(*it);
        if(phs)
        {
            if(assoc_disconn)
            {
                action_from_association *paaa = NULL;
                ACTION_TYPE type = NO_ACTION;
                if(auto_publish != "MANUAL") type = BURN_PER_STUDY;
                paaa = new action_from_association(type, get_path(), disconn_release, pflog);
                DWORD gle = phs->append_action(*paaa);
                delete paaa;
            }
            else time_header_out(*pflog) << "handle_dir::broadcast_action_to_all_study() " << *it << " : association is not disconnected." << endl;
        }
        else time_header_out(*pflog) << "handle_dir::broadcast_action_to_all_study() can't find study " << *it << endl;
    }
}

handle_proc& handle_proc::operator=(const handle_proc &r)
{
    meta_notify_file::operator=(r);
    hlog = r.hlog;
    exec_cmd = r.exec_cmd;
    exec_name = r.exec_name;
    log_path = r.log_path;
    procinfo = r.procinfo;
    return *this;
}

void handle_proc::print_state() const
{
    *pflog << "handle_proc::print_state()" << endl
        << "\texec_name: " << exec_name << endl
        << "\tlog_path: " << log_path << endl
        << "\texec_cmd: " << exec_cmd << endl;
    meta_notify_file::print_state();
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
        return displayErrorToCerr(__FUNCSIG__, GetLastError(), &flog);
    }
}

handle_compress* handle_compress::make_handle_compress(const NOTIFY_FILE_CONTEXT &nfc, ostream &flog)
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

    return new handle_compress(nfc.handle_dir_ptr, cmd, "dcmcjpeg", nfc, &flog);
}

handle_compress& handle_compress::operator=(const handle_compress &r)
{
    handle_proc::operator=(r);
    notify_ctx = r.notify_ctx;
    return *this;
}

void handle_compress::print_state() const
{
    *pflog << "handle_compress::print_state() " << notify_ctx.file.unique_filename << endl
        << "\tnotify_ctx.src_notify_filename: " << notify_ctx.src_notify_filename << endl
        << "\tnotify_ctx.file_seq: " << hex << setw(8) << setfill('0') << uppercase << notify_ctx.file_seq << endl
        << "\tnotify_ctx.file.studyUID: " << notify_ctx.file.studyUID << endl;
    handle_proc::print_state();
}

handle_study& handle_study::operator=(const handle_study &r)
{
    handle_proc::operator=(r);
    pipe_context = r.pipe_context;
    blocked = r.blocked;
    last_idle_time = r.last_idle_time;
    last_association_action = r.last_association_action;
    study_uid = r.study_uid;
    dicomdir_path = r.dicomdir_path;
    copy(r.set_association_path.begin(), r.set_association_path.end(), inserter(set_association_path, set_association_path.end()));
    copy(r.list_action.begin(), r.list_action.end(), back_inserter(list_action));
    return *this;
}

handle_study::~handle_study()
{
    if(pipe_context)
    {
        if(pipe_context->hPipeInst && pipe_context->hPipeInst != INVALID_HANDLE_VALUE)
        {
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" delete pipe context: " << pipe_context->study_uid << endl;
            if (! DisconnectNamedPipe(pipe_context->hPipeInst))
                displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError(), pflog);
            CloseHandle(pipe_context->hPipeInst);
        }
        delete pipe_context;
        pipe_context = NULL;
    }
    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ << endl;
}

DWORD handle_study::append_action(const action_from_association &action)
{
    list_action.push_back(action);
    if(blocked) return write_message_to_pipe();
    return 0;
}

DWORD handle_study::write_message_to_pipe()
{
    DWORD gle = 0;
    if(list_action.empty())
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" handle_study " << study_uid << " blocked, list_action is empty."  << endl;
        blocked = true;
        return gle;
    } // return error shall cause ~handle_study()

    list<action_from_association>::iterator it = list_action.begin();
    bool write_message_ok = false;
    while(!write_message_ok && it != list_action.end())
    {
        blocked = false;
        last_association_action = *it;
        time(&last_idle_time);
        switch(it->type)
        {
        case ACTION_TYPE::INDEX_INSTANCE:
            pipe_context->cbShouldWrite = sprintf_s(pipe_context->chBuffer, "%s|%s", it->pnfc->file.studyUID, it->pnfc->file.unique_filename);
            //replace(lpPipeInst->chBuffer, lpPipeInst->chBuffer + strlen(lpPipeInst->chBuffer), '/', '\\');

            if(!WriteFileEx(pipe_context->hPipeInst, pipe_context->chBuffer, pipe_context->cbShouldWrite, 
                (LPOVERLAPPED) pipe_context, (LPOVERLAPPED_COMPLETION_ROUTINE)named_pipe_server::write_pipe_complete_func_ptr))
            {   // return error shall cause ~handle_study()
                return displayErrorToCerr(__FUNCSIG__ " WriteFileEx()", GetLastError(), pflog);
            }
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" handle_study " << study_uid << " write message " << pipe_context->chBuffer << endl;
            write_message_ok = true;
            break;
        case ACTION_TYPE::BURN_PER_STUDY:
        case ACTION_TYPE::NO_ACTION:
            if(opt_verbose)
            {
                time_header_out(*pflog) << __FUNCSIG__" handle_study " << study_uid << " erase association " << it->get_path() << ", disconnect ";
                if(it->release) *pflog << " release." << endl;
                else *pflog << " abort." << endl;
            }

            set_association_path.erase(it->get_path());
            if(opt_verbose) time_header_out(*pflog) << "handle_study::write_message_to_pipe() erease_association " << study_uid << endl
                << "\tand erease assoc " << it->get_path() << endl;
            break;
        default:
            time_header_out(*pflog) << __FUNCSIG__" encounter " << action_from_association::translate_action_type(it->type) << " action." << endl;
            break;
        }
        it = list_action.erase(it);
    }
    if(!write_message_ok) blocked = true; // if(list_action is empty || last message is not INDEX_INSTANCE) blocked = true;
    return gle;
}

void handle_study::action_compress_ok(const string &filename, const string &xfer)
{
    list<action_from_association>::iterator it_clc = find_if(list_action.begin(), list_action.end(),
        [&filename](const action_from_association &lc){ return lc.pnfc && filename.compare(lc.pnfc->file.unique_filename) == 0; });
    if(it_clc != list_action.end())
    {
        cout << "trigger make_dicomdir " << study_uid << "\\" << it_clc->pnfc->file.filename << endl;

        strcpy_s(it_clc->pnfc->file.xfer_new, xfer.c_str());
        // todo: make_index(*it_clc);

        // send notification of a file dicomdir and index OK to state dir
        stringstream output;
        output << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPRESS_OK << endl;
        output << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << it_clc->pnfc->file_seq
            << " " << it_clc->pnfc->file.filename << " " << it_clc->pnfc->file.unique_filename << endl;
        output << NOTIFY_LEVEL_INSTANCE << " 00100020 ";
        x_www_form_codec_encode_to_ostream(it_clc->pnfc->file.patientID, &output);
        output << endl;
        output << NOTIFY_LEVEL_INSTANCE << " 0020000D " << it_clc->pnfc->file.studyUID << endl;
        output << NOTIFY_LEVEL_INSTANCE << " 0020000E " << it_clc->pnfc->file.seriesUID << endl;
        output << NOTIFY_LEVEL_INSTANCE << " 00080018 " << it_clc->pnfc->file.instanceUID << endl;
        output << NOTIFY_LEVEL_INSTANCE << " 00020010 " << it_clc->pnfc->file.xfer << " " << it_clc->pnfc->file.isEncapsulated << " " << it_clc->pnfc->file.xfer_new << endl;
        output << NOTIFY_LEVEL_PATIENT << " 00100010 ";
        x_www_form_codec_encode_to_ostream(it_clc->pnfc->patient.patientsName, &output);
        output << endl;
        output << NOTIFY_LEVEL_PATIENT << " 00100030 " << it_clc->pnfc->patient.birthday << endl;
        output << NOTIFY_LEVEL_PATIENT << " 00100040 " << it_clc->pnfc->patient.sex << endl;
        output << NOTIFY_LEVEL_PATIENT << " 00101020 " << it_clc->pnfc->patient.height << endl;
        output << NOTIFY_LEVEL_PATIENT << " 00101030 " << it_clc->pnfc->patient.weight << endl;
        output << NOTIFY_LEVEL_STUDY << " 00080020 " << it_clc->pnfc->study.studyDate << endl;
        output << NOTIFY_LEVEL_STUDY << " 00080030 " << it_clc->pnfc->study.studyTime << endl;
        output << NOTIFY_LEVEL_STUDY << " 00080050 ";
        x_www_form_codec_encode_to_ostream(it_clc->pnfc->study.accessionNumber, &output);
        output << endl;
        output << NOTIFY_LEVEL_SERIES << " 00080060 " << it_clc->pnfc->series.modality << endl;
        string notify = output.str();
        output.str("");

        char notify_file_name[MAX_PATH];
        size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), STATE_DIR);
        sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
        ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out);
        if(ntf.good())
        {
            ntf << notify ;
            ntf.close();
        }
        else time_header_out(*pflog) << notify;

        // this instance is all OK, erase it from queue
        list_action.erase(it_clc);
    }
    else time_header_out(*pflog) << __FUNCSIG__ " NamePipe read file's name is not in queue: " << filename << endl;
}

void handle_study::print_state() const
{
    *pflog << "handle_study::print_state() " << study_uid << endl
        << "\tdicomdir_path: " << dicomdir_path << endl
        << "\tblocked: " << blocked << endl
        << "\tlast_association_action: " << endl;
    last_association_action.print_state();
    // print associations
    for(set<string>::const_iterator it = set_association_path.begin(); it != set_association_path.end(); ++it)
        *pflog << "\tassociation_path " << *it << endl;
    // print actions
    *pflog << "\tactions:" << endl;
    for(list<action_from_association>::const_iterator it = list_action.begin(); it != list_action.end(); ++it)
    {
        *pflog << "\t" << action_from_association::translate_action_type(it->type);
        if(it->type == ACTION_TYPE::INDEX_INSTANCE)
        {
            if(it->pnfc) *pflog << " " << it->pnfc->file.unique_filename;
            else *pflog << " pnfc is NULL";
        }
        else if(it->type == ACTION_TYPE::BURN_MULTI) *pflog << it->burn_multi_id;
        *pflog << endl;
    }
    handle_proc::print_state();
}

bool handle_study::is_time_out() const
{
    time_t now = 0;
    time(&now);
    if(last_association_action.is_disconnect() && list_action.size() == 0 && set_association_path.size() == 0)
    {
        if(now - last_idle_time > 15) return true;
    }
    else if(now - last_idle_time > 600) return true;
    return false;
}

void action_from_association::print_state() const
{
    *pflog << "action_from_association::print_state()" << endl
        << "\ttype: " << action_from_association::translate_action_type(type) << endl
        << "\trelease: " << release << endl
        << "\tburn_multi_id: " << burn_multi_id << endl;
    if(pnfc)
    {
        *pflog << "\tNOTIFY_FILE_CONTEXT pnfc: " << pnfc->file.unique_filename << endl
        << "\tpnfc->src_notify_filename: " << pnfc->src_notify_filename << endl
        << "\tpnfc->file_seq: " << hex << setw(8) << setfill('0') << uppercase << pnfc->file_seq << endl
        << "\tpnfc->file.studyUID: " << pnfc->file.studyUID << endl;
    }
    base_path::print_state();
}

action_from_association& action_from_association::operator=(const action_from_association &r)
{
    base_path::operator=(r);
    type = r.type;
    release = r.release;
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
        if(type) return false;
        else
        {
            if(r.type) return true;
            else // type == false && r.type == false
            {
                if(get_path() < r.get_path()) return true;
                else if(r.get_path() < get_path()) return false;
                else // association_path == r.association_path
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
        }
    }
}
