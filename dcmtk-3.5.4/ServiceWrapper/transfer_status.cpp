#include "stdafx.h"

using namespace std;
using namespace handle_context;

const char* action_from_association::translate_action_type(ACTION_TYPE t)
{
    switch(t)
    {
    case NO_ACTION: return "NO_ACTION";
    case BURN_PER_STUDY: return "BURN_PER_STUDY";
    case BURN_MULTI: return "BURN_MULTI";
    case INDEX_INSTANCE: return "INDEX_INSTANCE";
    case NO_INSTANCE: return "NO_INSTANCE";
    default: return "ERROR_ACTION_TYPE";
    }
}

handle_dir& handle_dir::operator=(const handle_dir &r)
{
    base_dir::operator=(r);
    assoc = r.assoc;
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
    named_pipe_server *nps = named_pipe_server::get_named_pipe_server_singleton();
    if(nps)
    {
        string assoc_path(get_path());
        for_each(set_study.begin(), set_study.end(), [nps, &assoc_path](const string &study_uid) {
            handle_study *phs = nps->find_handle_study(study_uid);
            if(phs) phs->remove_association_path(assoc_path);
        });
    }

    if(get_notify_filename().length())
    {
        char newname[MAX_PATH];
        strcpy_s(newname, get_notify_filename().c_str());
        char *p = strrchr(newname, '.');
        if(p)
        {
            ++p;
            *p = '\0';
            strcat_s(newname, "txt");
            ofstream ofs_txt(newname);
            if(ofs_txt.fail())
            {
                char msg[1024];
                strerror_s(msg, errno);
                time_header_out(*pflog) << __FUNCSIG__" create complete file " << newname << " failed: " << msg << endl;
            }
            else
            {
                ofs_txt << close_description() << endl;
                ofs_txt.close();
            }
        }
    }
}

void handle_dir::print_state() const
{
    *pflog << "handle_dir::print_state()" << endl
        << "\tstore_assoc_id: " << assoc.store_assoc_id << endl
        << "\tcallingAE: " << assoc.callingAE << endl
        << "\tcallingAddr: " << assoc.callingAddr << endl
        << "\tcalledAE: " << assoc.calledAE << endl
        << "\tcalledAddr: " << assoc.calledAddr << endl
        << "\texpected_xfer: " << assoc.expected_xfer << endl
        << "\tauto_publish: " << assoc.auto_publish << endl
        << "\tport: " << dec << assoc.port << endl
        << "\tlast_association_notify_filename: " << last_association_notify_filename << endl
        << "\tassoc_disconn: " << assoc_disconn << endl
        << "\tdisconn_release: " << disconn_release << endl
        << "\tlast_find_error: " << last_find_error << endl;
    set<string> remain(set_complete);
    *pflog << "\tassociation " << get_id() << " " << get_path() << " file " << dec << list_file.size() << ", complete " << set_complete.size() << endl;
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

    base_dir::print_state();
}

string& handle_dir::get_find_filter(std::string &filter) const
{
    filter = get_path();
    if(get_id().length()) filter.append("\\state"); // is association
    // else is store_notify
    return filter.append("\\*.dfc");
}

DWORD handle_dir::find_files(std::ostream &flog, std::function<DWORD(const std::string&)> pred)
{
    struct _finddata_t wfd;
    string filter;

    refresh_last_access(); 

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

    // handle OK, process new files.
    new_files.sort();
    for(list<string>::iterator it = new_files.begin(); it != new_files.end(); ++it)
    {
        DWORD gle = pred(*it);
        if(gle)
        {
            if(gle == ERROR_SHARING_VIOLATION)
            {
                process_file_error = true;
                break; // try it later
            }
            else if(gle == ERROR_DATATYPE_MISMATCH)
                continue; // ACKN file or file has been processed
            else
                displayErrorToCerr(__FUNCSIG__" pred(it)", gle, pflog); // other error
        }
        else list_file.push_back(*it); // success
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
    if(strcmp(pclc->file.charset, "ISO_IR 192") == 0)
    {
        char pn[128];
        if(UTF8ToGBK(pclc->patient.patientsName, pn, sizeof(pn)))
            strcpy_s(pclc->patient.patientsName, pn);
        else
            time_header_out(flog) << __FUNCSIG__" convert from utf-8 fail: " << pclc->patient.patientsName << endl;
    }
    if(cmd) delete[] cmd;
    return pclc;
}

void handle_dir::process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog)
{
    switch(tag)
    {
    case NOTIFY_ASSOC_ESTA:
        strcpy_s(assoc.id, get_id().c_str());
        strcpy_s(assoc.path, get_path().c_str());
        ifs >> assoc.store_assoc_id >> assoc.callingAE >> assoc.callingAddr >> assoc.calledAE >> dec >> assoc.port >> assoc.expected_xfer >> assoc.auto_publish >> assoc.calledAddr;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << assoc.id << " "
            << assoc.callingAE << " " << assoc.callingAddr<< " " << assoc.calledAE<< " " << dec << assoc.port << " " << assoc.expected_xfer << " "<< assoc.calledAddr << endl;
        break;
    case NOTIFY_ASSOC_RELEASE:
        assoc_disconn = true;
        disconn_release = true;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_id() << " disconnect release." << endl;
        break;
    case NOTIFY_ASSOC_ABORT:
        assoc_disconn = true;
        disconn_release = false;
        if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify_association() " << get_id() << " disconnect abort." << endl;
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

DWORD handle_dir::process_notify(const std::string &filename, NOTIFY_LIST &compress_queue, std::ostream &flog)
{
    DWORD gle = 0, tag;
    string cmd, filepath(get_path());
    filepath.append("\\state\\").append(filename);
    if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify(" << filename << "): " << get_id() << ":" << filepath << endl;

    ifstream ifs(filepath, ios_base::in, _SH_DENYWR);
    if(ifs.fail())
    {
        gle = GetLastError();
        if(gle != ERROR_SHARING_VIOLATION || opt_verbose)
        {
            string msg("handle_dir::process_notify() open file failed: ");
            msg.append(filepath);
            return displayErrorToCerr(msg.c_str(), gle, &flog);
        }
        else return gle;
    }

    refresh_last_access();

    ifs >> cmd >> hex >> tag;
    
    if(cmd.compare(NOTIFY_FILE_TAG) == 0) // receive a file
    {
        NOTIFY_FILE_CONTEXT *pnfc = process_notify_file(ifs, tag, flog);
        if(pnfc)
        {
            this->fill_association_section(pnfc->assoc);
            strcpy_s(pnfc->src_notify_filename, filename.c_str());
            NotifyFileContextStorePath(&pnfc->file);
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify(" << filename << ") " << get_id() << " read OK." << endl;
#ifdef _DEBUG
            time_header_out(cerr) << "handle_dir::process_notify(" << filename << ") " << get_id() << " read OK." << endl;
#endif
            // don't insert file to set_complete now, insert it after compress complete
            compress_queue.push_back(*pnfc);
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
        if(tag == NOTIFY_ASSOC_ESTA) last_association_notify_filename = filename;
        set_complete.insert(filename);
    }
    else if(cmd.compare(NOTIFY_ACKN_ITEM) == 0)
    {
        if(tag == NOTIFY_COMPRESS_OK || tag == NOTIFY_COMPRESS_FAIL)
        {
            string src_notify_file;
            ifs >> src_notify_file;
            set_complete.insert(src_notify_file);
            set_complete.insert(filename); // complete ack self
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify() recieve compress complete notify " << filename << "(" << src_notify_file << ")." << endl;
        }
        else if(tag == NOTIFY_ALL_COMPRESS_OK)
        {
            set_complete.insert(filename);
            if(opt_verbose) time_header_out(flog) << "handle_dir::process_notify() recieve all compress complete notify " << filename << endl;
        }
        else time_header_out(flog) << "handle_dir::process_notify() ignore ack file " << filename << endl
            << cmd << " " << hex << uppercase << tag << " ..." << endl;
    }
    else time_header_out(flog) << "handle_dir::process_notify() ignore " << filename << endl;

    if(ifs.is_open()) ifs.close();
    return gle;
}

void handle_dir::send_compress_complete_notify(const NOTIFY_FILE_CONTEXT &nfc, bool compress_ok, ostream &flog)
{
    refresh_last_access();

    char notify_file_name[MAX_PATH];
    string prefix(get_path());
    prefix.append("\\state\\");
    size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), prefix.c_str());
    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out, _SH_DENYWR);
    if(ntf.good())
    {
        save_notify_context_to_ostream(nfc, compress_ok, ntf);
        ntf.close();
        if(opt_verbose) time_header_out(flog) << "handle_dir::send_compress_complete_notify() to " << notify_file_name << " OK." << endl;
    }
    else
    {
        time_header_out(flog) << "handle_dir::send_compress_complete_notify() to " << notify_file_name << " failed:" << endl;
        save_notify_context_to_ostream(nfc, compress_ok, flog);
    }
}

void handle_dir::send_all_compress_ok_notify()
{
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
        if(opt_verbose) time_header_out(*pflog) << "watch_notify() send all compress OK notify " << notify_file_name << " OK." << endl;
    }
    else
    {
        time_header_out(*pflog) << "watch_notify() send all compress OK notify " << notify_file_name << " failed:" << endl;
        *pflog << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_ALL_COMPRESS_OK << endl;
    }
}

void handle_dir::broadcast_assoc_close_action_to_all_study(named_pipe_server &nps) const
{
    action_from_association *paaa = NULL;
    ACTION_TYPE type = NO_ACTION;
    if(is_normal_close())
    {
        if(strcmp(assoc.auto_publish, "STUDY") == 0) type = BURN_PER_STUDY;
    }
    else
        time_header_out(*pflog) << "handle_dir::broadcast_assoc_close_action_to_all_study() failed: association " << get_id() << " is not disconnected, force close." << endl;

    paaa = new action_from_association(type, get_path(), disconn_release, pflog);

    for(set<string>::iterator it = set_study.begin(); it != set_study.end(); ++it)
    {
        handle_study *phs = nps.find_handle_study(*it);
        if(phs)
        {
            DWORD gle = phs->append_action(*paaa);
        }
        else time_header_out(*pflog) << "handle_dir::broadcast_assoc_close_action_to_all_study() can't find study " << *it << endl;
    }
    if(paaa) delete paaa;
}

const char* handle_dir::close_description() const
{
    if(assoc_disconn)
    {
        if(disconn_release) return "OK";
        else return "ABORT";
    }
    else if(is_time_out()) return "TIMEOUT";
    else return "UNKNOWN";
}

handle_study::handle_study(const std::string &cwd, const std::string &cmd, const std::string &exec_prog_name, const std::string &study, std::ostream *plog)
    : handle_proc(study, cwd, "", cmd, exec_prog_name, plog), pipe_context(NULL), blocked(false),
    ris_integration_start(false), last_association_action(INDEX_INSTANCE, cwd, false, plog)
{
    char seq_buff[MAX_PATH];
    size_t pos = in_process_sequence_dll(seq_buff, sizeof(seq_buff), "");
    sprintf_s(seq_buff + pos, sizeof(seq_buff) - pos, "_%s", get_id());
    lock_file_name = seq_buff;
}

handle_study& handle_study::operator=(const handle_study &r)
{
    handle_proc::operator=(r);
    pipe_context = r.pipe_context;
    blocked = r.blocked;
    ris_integration_start = r.ris_integration_start;
    last_association_action = r.last_association_action;
    lock_file_name = r.lock_file_name;
    copy(r.set_association_path.begin(), r.set_association_path.end(), inserter(set_association_path, set_association_path.end()));
    copy(r.list_action.begin(), r.list_action.end(), back_inserter(list_action));
    return *this;
}

handle_study::~handle_study()
{
    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ << " " << get_id() << endl;
    if(pipe_context)
    {
        if(pipe_context->hPipeInst && pipe_context->hPipeInst != INVALID_HANDLE_VALUE)
        {
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" delete pipe context " << pipe_context->hPipeInst
                << ", " << pipe_context->study_uid << endl;

            DWORD cbWritten = 0;
            string close_command;
            if(last_association_action.is_auto_publish())
                close_command = "close study ";
            else
                close_command = "close manual ";
            close_command.append(lock_file_name);
            WriteFile(pipe_context->hPipeInst, close_command.c_str(), close_command.length(), &cbWritten, NULL);

            if (! DisconnectNamedPipe(pipe_context->hPipeInst))
                displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError(), pflog);
            CloseHandle(pipe_context->hPipeInst);
        }
        delete pipe_context;
        pipe_context = NULL;
    }
    //xml_index::singleton_ptr->unload_and_sync_study(study_uid);
}

DWORD handle_study::append_action(const action_from_association &action)
{
    refresh_last_access();
    list_action.push_back(action);
    if(blocked) return write_message_to_pipe();
    return 0;
}

static int CreateMapFieldsLocal(map<string, string> &map_field, istream &ff, ostream &index_log, bool opt_verbose)
{
    char line[1024];
    ff.getline(line, sizeof(line));
    int cnt = 0;
    while(ff.good())
    {
        if(strlen(line) <= 0) goto next_line;
        char *p = strchr(line, '=');
        if(p)
        {
            *p++ = '\0';
            map_field[line] = p;
        }
        else
        {
            map_field[line] = string();
        }
#ifdef _DEBUG
        if(opt_verbose) index_log << "CreateMapFieldsLocal() " << line << " " << p << endl;
#endif
        ++cnt;
next_line:
        ff.getline(line, sizeof(line));
    }
    return cnt;
}

bool handle_study::open_study_lock_file(map<string, string> &map_ini)
{
    string study_notify_path(GetPacsBase());
    study_notify_path.append("\\orders_study\\").append(lock_file_name).append(".ini");

    if(_access(study_notify_path.c_str(), 6))
    {
        errno_t en = errno;
        if(en == ENOENT) return true;
        else
        {
            char msg[1024];
            strerror_s(msg, en);
            time_header_out(*pflog) << __FUNCSIG__" failed: access " << study_notify_path << " : " << msg << endl;
        }
    }
    else
    {
        ifstream sfs(study_notify_path.c_str(), ios_base::in, _SH_DENYWR);
        if(sfs.good())
        {
            CreateMapFieldsLocal(map_ini, sfs, *pflog, opt_verbose != 0);
            sfs.close();
            return true;
        }
        else
        {
            string msg(__FUNCSIG__" open ");
            displayErrorToCerr(msg.append(study_notify_path).c_str(), GetLastError(), pflog);
        }
    }

    return false;
}

void handle_study::save_and_close_lock_file(std::map<std::string, std::string> &map_ini)
{
    string study_notify_path(GetPacsBase());
    study_notify_path.append("\\orders_study\\").append(lock_file_name).append(".ini");
    ofstream sfs(study_notify_path, ios_base::out | ios_base::trunc, _SH_DENYWR);
    if(sfs.good())
    {
        map<string, string>::iterator itm = map_ini.begin();
        while(itm != map_ini.end())
        {
            if(itm->second.length())
            {
                sfs << itm->first << "=" << itm->second << endl;
                ++itm;
            }
            else
            {
                time_header_out(*pflog) << __FUNCSIG__" state empty error: " << itm->first << endl;
                itm = map_ini.erase(itm);
            }
        }
        sfs.close();
    }
    else
        time_header_out(*pflog) << __FUNCSIG__" failed: can't refresh " << study_notify_path << endl;
}

bool handle_study::insert_association_path(const std::string &assoc_path)
{
    refresh_last_access();

    bool insert_result = set_association_path.insert(assoc_path).second;
	if(insert_result)
	{
		map<string, string> map_ini;
		if(open_study_lock_file(map_ini))
		{
			for(set<string>::const_iterator it = set_association_path.cbegin(); it != set_association_path.cend(); ++it)
			{
				map<string, string>::iterator itm = map_ini.find(*it);
				if(itm == map_ini.end()) map_ini[*it] = "ALIVE";
			}
			save_and_close_lock_file(map_ini);
		}
	}
    return insert_result;
}

void handle_study::remove_association_path(const string &assoc_path)
{
    refresh_last_access();

    set_association_path.erase(assoc_path);

    map<string, string> map_ini;
    bool loadOK = open_study_lock_file(map_ini);
    
    map<string, string>::iterator itm = map_ini.find(assoc_path);
    if(itm != map_ini.end()) map_ini[assoc_path] = "OFF";

    if(loadOK) save_and_close_lock_file(map_ini);
}

DWORD handle_study::write_message_to_pipe()
{
    DWORD gle = 0;
    if(list_action.empty())
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" handle_study " << get_id() << " blocked, list_action is empty."  << endl;
        blocked = true;
        return gle;
    } // return error shall cause ~handle_study()

    list<action_from_association>::iterator it = list_action.begin();
    bool write_message_ok = false;
    while(!write_message_ok && it != list_action.end())
    {
        blocked = false;
        last_association_action = *it;
        
        if(opt_verbose)
        {
            *pflog << "update ";
            last_association_action.print_state();
        }

        refresh_last_access();
        switch(it->type)
        {
        case INDEX_INSTANCE:
            pipe_context->cbShouldWrite = sprintf_s(pipe_context->chBuffer, "%s|%s|%lld\n%s %s %s %s %s %s %s %s %d %s %s %s",
                it->pnfc->file.studyUID, it->pnfc->file.unique_filename, it->pnfc->file.file_size_receive,
                it->pnfc->assoc.path, it->pnfc->file.filename, it->pnfc->file.xfer, it->pnfc->assoc.id, it->pnfc->assoc.store_assoc_id,
                it->pnfc->assoc.callingAE, it->pnfc->assoc.callingAddr, it->pnfc->assoc.calledAE, it->pnfc->assoc.port,
                it->pnfc->assoc.expected_xfer, it->pnfc->assoc.auto_publish, it->pnfc->assoc.calledAddr);
            //replace(lpPipeInst->chBuffer, lpPipeInst->chBuffer + strlen(lpPipeInst->chBuffer), '/', '\\');

            if(!WriteFileEx(pipe_context->hPipeInst, pipe_context->chBuffer, pipe_context->cbShouldWrite, 
                (LPOVERLAPPED) pipe_context, (LPOVERLAPPED_COMPLETION_ROUTINE)named_pipe_server::write_pipe_complete_func_ptr))
            {   // return error shall cause ~handle_study()
                return displayErrorToCerr(__FUNCSIG__ " WriteFileEx()", GetLastError(), pflog);
            }
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" handle_study " << get_id() << " write message " << pipe_context->chBuffer << endl;
            write_message_ok = true;
            break;
        case BURN_PER_STUDY:
        case NO_ACTION:
            if(opt_verbose)
            {
                time_header_out(*pflog) << __FUNCSIG__" handle_study " << get_id() << " erase association " << it->get_path() << ", disconnect ";
                if(it->release) *pflog << " release." << endl;
                else *pflog << " abort." << endl;
            }

            remove_association_path(it->get_path());
            if(opt_verbose) time_header_out(*pflog) << "handle_study::write_message_to_pipe() erease_association " << get_id() << endl
                << "\tand erease assoc " << it->get_path() << endl;
            break;
        case NO_INSTANCE:
            // instance file is not found, erase it only.
            break;
        default:
            time_header_out(*pflog) << __FUNCSIG__" encounter " << action_from_association::translate_action_type(it->type) << " action." << endl;
            break;
        }
        if(!write_message_ok) it = list_action.erase(it); // don't delete INDEX_INSTANCE action, action_compress_ok shall use it
    }
    if(!write_message_ok) blocked = true; // if(list_action is empty || last message is not INDEX_INSTANCE) blocked = true;
    return gle;
}

void handle_study::remove_compress_ok_action(const string &filename, const string &xfer)
{
    refresh_last_access();

    list<action_from_association>::iterator it_clc = find_if(list_action.begin(), list_action.end(),
        [&filename](const action_from_association &lc){ return lc.pnfc && filename.compare(lc.pnfc->file.unique_filename) == 0; });
    if(it_clc != list_action.end())
    {
        //cout << "trigger make_dicomdir " << study_uid << "\\" << it_clc->pnfc->file.filename << endl;

        strcpy_s(it_clc->pnfc->file.xfer_new, xfer.c_str());

        if(!ris_integration_start) // start handle_ris_integration once per study.
        {
            char ris_integration_prog[MAX_PATH];
            if(GetSetting("RisIntegration", ris_integration_prog, sizeof(ris_integration_prog)))
            {
                ris_integration_start = handle_proc::make_proc_ris_integration(it_clc->pnfc, ris_integration_prog, *pflog);
                if(!ris_integration_start) time_header_out(*pflog) << __FUNCSIG__" start ris integration failed." << endl;
            }
        }
        // this instance is all OK, erase it from queue
        list_action.erase(it_clc);
    }
    else time_header_out(*pflog) << __FUNCSIG__ " NamePipe read file's name is not in queue: " << filename << endl;
}

void handle_study::print_state() const
{
    *pflog << "handle_study::print_state() " << get_id() << endl
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
        if(it->type == INDEX_INSTANCE)
        {
            if(it->pnfc) *pflog << " " << it->pnfc->file.unique_filename;
            else *pflog << " pnfc is NULL";
        }
        else if(it->type == BURN_MULTI) *pflog << it->burn_multi_id;
        *pflog << endl;
    }
    handle_proc::print_state();
}

bool handle_study::is_time_out() const
{
    time_t diff = 0;
    time(&diff);
    diff -= get_last_access();
    
    if(last_association_action.is_disconnect() && list_action.size() == 0 && set_association_path.size() == 0)
    {
        if(diff > store_timeout) return true;
    }
    else if(diff > get_timeout()) return true;
    return false;
}

bool handle_study::send_remain_message_to_pipe()
{
    if(list_action.size())
    {
        if(blocked) write_message_to_pipe();
        return true;
    }
    else return !blocked;
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
        if(pnfc == NULL) pnfc = new NOTIFY_FILE_CONTEXT;
        *pnfc = *r.pnfc;
    }
    else
    {
        if(pnfc) delete pnfc;
        pnfc = NULL; // r.pnfc is NULL
    }
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
