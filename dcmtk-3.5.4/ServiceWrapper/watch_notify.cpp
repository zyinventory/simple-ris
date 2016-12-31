#include "stdafx.h"
#include "commonlib.h"
#include "named_pipe/named_pipe_listener.h"

using namespace std;
using namespace handle_context;

static char buff[FILE_BUF_SIZE];
static const string debug_mode_header("DebugMode");

static HANDLE_MAP map_handle_context;
static HANDLE_DIR_MAP handle_dir_map;
static HANDLE_PROC_LIST proc_list;
static NOTIFY_LIST compress_queue;

class my_np_conn : public named_pipe_connection
{
private:
    DWORD process_file_incoming(const char *assoc_id, const char *notify_file);
    handle_dir* create_new_handle_dir(const char *assoc_id);

public:
    my_np_conn(named_pipe_listener *pnps) : named_pipe_connection(pnps) { };
    virtual ~my_np_conn() { };
    virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
};

DWORD my_np_conn::process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size)
{
    if(cbBytesRead) time_header_out(*pflog) << "my_np_conn::process_message(): " << ptr_data_buffer << endl;
    else time_header_out(*pflog) << "my_np_conn::process_message(): cbBytesRead is 0" << endl;

    // process message, must retrieve ack message unless ptr_data_buffer == "close_pipe"
    // if(gle) void CALLBACK handle_context::read_pipe_complete(...) shall close pipe
    if(strncmp(ptr_data_buffer, "close_pipe", 10) == 0) return ERROR_HANDLE_EOF;

    char *notify_file = strchr(ptr_data_buffer, ';');
    if(notify_file == NULL)
        time_header_out(*pflog) << "my_np_conn::process_message() receive an unknown message: " << ptr_data_buffer << endl;
    else
    {
        *notify_file++ = '\0';
        process_file_incoming(ptr_data_buffer, notify_file);
    }
    return 0;
}

handle_dir* my_np_conn::create_new_handle_dir(const char *p_assoc_id)
{
    string meta_notify_file(GetPacsTemp());
    meta_notify_file.append("\\pacs\\"NOTIFY_BASE"\\").append(p_assoc_id).append("_ACKN.dfc");
#ifdef _DEBUG
    time_header_out(cerr) << "my_np_conn::process_meta_notify_file() process meta notify: " << meta_notify_file << endl;
#endif
    ifstream ntff(meta_notify_file, ios_base::in, _SH_DENYWR);
    if(ntff.fail())
    {
        DWORD gle = GetLastError();
        string msg("my_np_conn::process_meta_notify_file() open file ");
        msg.append(meta_notify_file);
        displayErrorToCerr(msg.c_str(), gle, pflog);
        return NULL;
    }
    string cmd, path, assoc_id, calling, called, remote, port, transfer_syntax, auto_publish;
    DWORD tag, pid, gle = 0;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port >> transfer_syntax >> auto_publish;
    if(ntff.is_open()) ntff.close();
#ifdef _DEBUG
    time_header_out(cerr) << cmd << " " << hex << tag << " " << path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;
#endif
    path.insert(0, "\\pacs\\").insert(0, GetPacsTemp());
    return new handle_dir(p_assoc_id, path, meta_notify_file, pflog);
}

DWORD my_np_conn::process_file_incoming(const char *p_assoc_id, const char *notify_file)
{
#ifdef _DEBUG
    time_header_out(cerr) << "my_np_conn::process_file_incoming() receive association notify " << p_assoc_id << ";"<< notify_file << endl;
#endif
    HANDLE_DIR_MAP::iterator it = handle_dir_map.find(p_assoc_id);
    handle_dir *phd = NULL;
    if(it != handle_dir_map.end()) phd = it->second;
    if(phd == NULL)
    {
        set_id(p_assoc_id);
        set_meta_notify_filename(notify_file);
        phd = create_new_handle_dir(p_assoc_id);
        if(phd) handle_dir_map[p_assoc_id] = phd;
    }
    if(phd) phd->process_notify(notify_file, compress_queue, *pflog);
    else time_header_out(*pflog) << "my_np_conn::process_file_incoming() can't create handle_dir()" << endl;
    return 0;
}

static bool select_handle_dir_by_association_path(const handle_compress *pnf, const string &association_id, const string &path, ostream &flog)
{
    if(pnf == NULL) return false;
    bool same_assoc = (association_id.compare(pnf->get_id()) == 0);
    bool same_path = (path.compare(pnf->get_path()) == 0);

    if(same_assoc && same_path)
        return (NULL != dynamic_cast<const handle_dir*>(pnf));
    else if(same_assoc || same_path)
    {
        time_header_out(flog) << "select_handle_dir_by_association_path(" << association_id << ", " << path 
            << ") mismatch (" << pnf->get_id() << ", " << pnf->get_path() << ")" << endl;
        // path is unique key
        if(same_path) return (NULL != dynamic_cast<const handle_dir*>(pnf));
    }
    return false;
}

static void write_association_complete_text_file(const string &meta_notify_file_txt, const char *msg, DWORD gle, ostream &flog)
{
    ofstream ofstxt(meta_notify_file_txt); // ignore this transfer
    if(ofstxt.fail())
    {
        DWORD gle2 = GetLastError();
        char buff2[1024];
        sprintf_s(buff2, "write_association_complete_text_file() create complete text %s", meta_notify_file_txt.c_str());
        displayErrorToCerr(buff2, gle2, &flog);
    }
    else
    {
        if(gle)
            displayErrorToCerr(msg, gle, &ofstxt);
        else
            ofstxt << msg << endl;
        ofstxt.close();
    }
}

static DWORD disable_remained_meta_notify_file(const char *pattern, ostream &flog)
{
    struct _finddata_t wfd;
    string filter(pattern);
    intptr_t hSearch = _findfirst(filter.c_str(), &wfd);
    if(hSearch == -1)
    {
        errno_t en = errno;
        if(en == ENOENT) return 0;
        else
        {
            strerror_s(buff, en);
            if(opt_verbose) time_header_out(flog) << "disable_remained_meta_notify_file() " << filter << " failed: " << buff << endl;
            return -1;
        }
    }

    string::size_type pos = filter.rfind('*');
    filter.erase(pos);
    do {
        string node(wfd.name);
        if (node.compare(".") == 0 || node.compare("..") == 0) 
			continue; // skip . .. DICOMDIR
        if((wfd.attrib & _A_SUBDIR) == 0)
        {
            node.insert(node.begin(), filter.cbegin(), filter.cend());
            pos = node.rfind('.');
            node.erase(pos).append(".txt");
            if(0 != _access(node.c_str(), 0))
                write_association_complete_text_file(node, "discard", 0, flog);
        }
	} while(_findnext(hSearch, &wfd) == 0);
	_findclose(hSearch);
    return 0;
}

static void compress_complete(const string &assoc_id, const string &study_uid, bool compress_ok,
    named_pipe_server &nps, NOTIFY_FILE_CONTEXT &nfc, ostream &flog)
{
    if(opt_verbose) time_header_out(flog) << "compress_complete() dcmcjpeg " << (compress_ok ? "complete" : "failed")
        << ", find association " << assoc_id << " to set complete." << endl;

    handle_dir *phd = NULL;
    HANDLE_DIR_MAP::iterator it = handle_dir_map.find(assoc_id);
    if(it != handle_dir_map.end() && it->second) phd = it->second;
	if(phd == NULL) time_header_out(flog) << "compress_complete() can't find association " << assoc_id << ", src file " << nfc.src_notify_filename << endl;

    handle_study* phs = nps.find_handle_study(study_uid);
	if(phs == NULL)
	{
		phs = nps.make_handle_study(study_uid);
		if(phs == NULL)
			time_header_out(flog) << "compress_complete() can't create study " << study_uid << ", src file " << nfc.src_notify_filename << endl;
	}

    if(phs)
    {
        phs->insert_association_path(nfc.assoc.path);  // add association lock to study, study[1] -> association[n]
        if(compress_ok) phs->append_action(action_from_association(nfc, nfc.assoc.path, &flog));
        else
        {
            time_header_out(flog) << "compress_complete() compress_ok is false, skip append_action(action_from_association()):" << endl;
            save_notify_context_to_ostream(nfc, compress_ok, flog);
        }
    }
    else
    {
        time_header_out(flog) << "compress_complete() can't find study " << study_uid 
            << ", skip insert_association_path() and append_action(action_from_association()):" << endl;
        save_notify_context_to_ostream(nfc, compress_ok, flog);
    }

    if(phd)
    {
        // establish relationship from association to study
        phd->insert_study(study_uid); // association[1] -> study[n]
        phd->send_compress_complete_notify(nfc, compress_ok, flog); // phs->append_action(action_from_association(nfc));
    }
    else
    {
        time_header_out(flog) << "compress_complete() can't find association " << assoc_id 
            << ", skip insert_study() and send_compress_complete_notify():" << endl;
        save_notify_context_to_ostream(nfc, compress_ok, flog);
    }
}

static bool close_handle_dir(handle_dir *phdir, handle_dir *pclz_base_dir, named_pipe_server &nps, bool pick_up, ostream &flog)
{
    if(phdir && phdir->get_id().length())
    {
        for(HANDLE_MAP::iterator it = map_handle_context.begin(); it != map_handle_context.end(); ++it)
        {
            handle_compress *phcompr = it->second;
            if(phcompr && phcompr->get_id() == phdir->get_id()) return false;
        }

        for(NOTIFY_LIST::iterator it = compress_queue.begin(); it != compress_queue.end(); ++it)
        {
            if(phdir->get_id().compare(it->assoc.id) == 0) return false;
        }

        if(phdir->is_association_disconnect() || phdir->is_time_out())
        {
            if(opt_verbose)
            {
                time_header_out(flog) << "close_handle_dir()" << (pick_up ? " pick up" : "") << " association " << phdir->get_id() << " complete, erease from map_handle_context." << endl;
                flog << "\t" << phdir->close_description() << " close." << endl;
            }
            // close monitor handle, all_compress_ok_notify is not processed.
            phdir->send_all_compress_ok_notify();
            phdir->broadcast_assoc_close_action_to_all_study(nps);

            // todo: need meta_handle_dir
            if(pclz_base_dir) pclz_base_dir->remove_file_from_list(phdir->get_meta_notify_filename());

            if(opt_verbose || phdir->file_complete_remain())
                time_header_out(flog) << "close_handle_dir() handle_dir" << (pick_up ? " pick up" : "") << " exit:" << endl;

            time_header_out(flog) << "close_handle_dir() delete handle_dir:" << endl;
            phdir->print_state();
            delete phdir;

            return true;
        }
    }
    return false;
}

static bool handle_less(const BASE_HANDLE_PAIR &p1, const BASE_HANDLE_PAIR &p2)
{
	if(p1.second == NULL && p2.second == NULL) return p1.first < p2.first;
	else if(p2.second == NULL) return true;
	else if(p1.second == NULL) return false;

	int c1 = 3, c2 = 3;

    if(c1 == 1) // p1 and p2 is handle_dir
    {
        if(p1.second->get_id().length() == 0 && p2.second->get_id().length() > 0) return true;
        else if(p2.second->get_id().length() == 0 && p1.second->get_id().length() > 0) return false;
        // else (p1 and p2 length > 0) || (p1 and p2 length == 0), compare last_access
    }
    return p1.second->get_last_access() < p2.second->get_last_access();
}

static named_pipe_connection* WINAPI create_new_pipe_connect(named_pipe_listener *pnps)
{
    return new my_np_conn(pnps);
}

int watch_notify(string &cmd, ofstream &flog)
{
    sprintf_s(buff, "%s\\pacs\\"NOTIFY_BASE"\\*.dfc", GetPacsTemp());
    disable_remained_meta_notify_file(buff, flog);
    sprintf_s(buff, "%s\\orders_study\\*.ini", GetPacsTemp());
    disable_remained_meta_notify_file(buff, flog);

    // listen on named pipe
    named_pipe_listener qrnps("\\\\.\\pipe\\dcmtk_qr", PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, create_new_pipe_connect, &flog);
    DWORD gle = qrnps.start_listening();
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        displayErrorToCerr("named_pipe_listener.start_listening()", gle, &cerr);
        return gle;
    }

    // start dcmqrscp.exe parent proc
    sprintf_s(buff, "%s\\pacs", GetPacsBase());
    handle_proc *phproc = new handle_proc("dcmqrscp", buff, "", cmd, "dcmqrscp", &flog);
    gle = phproc->start_process(true);
    if(gle)
    {
        displayErrorToCerr("watch_notify() handle_proc::create_process(dcmqrscp) at beginning", gle, &flog);
        return gle;
    }
    if(opt_verbose) time_header_out(flog) << "watch_notify() dcmqrscp start" << endl;
#ifdef _DEBUG
    strcpy_s(buff, __FILE__);
    char *p = strrchr(buff, '\\');
    if(p)
    {
        *p = '\0';
        p = strrchr(buff, '\\');
        if(p)
        {
            *p = '\0';
            strcat_s(buff, "\\Debug\\jobloader.exe");
        } else sprintf_s(buff, "%s\\bin\\jobloader.exe", GetPacsBase());
    } else sprintf_s(buff, "%s\\bin\\jobloader.exe", GetPacsBase());
#else
    sprintf_s(buff, "%s\\bin\\jobloader.exe", GetPacsBase());
#endif
    cmd = buff;
    sprintf_s(buff, "%s\\pacs", GetPacsBase());
    handle_proc *phproc_job = new handle_proc("jobloader", buff, "", cmd, "jobloader", &flog);
    gle = phproc_job->start_process(false);
    if(gle)
    {
        displayErrorToCerr("watch_notify() handle_proc::create_process(jobloader) at beginning", gle, &flog);
        return gle;
    }
    if(opt_verbose) time_header_out(flog) << "watch_notify() jobloader start" << endl;
    
    if(GetSetting(debug_mode_header, buff, sizeof(buff)))
    {
        int debug_flag = atoi(buff);
        if(debug_flag) debug_mode = true;
    }

    //xml_index xi(&flog);
    //xml_index::singleton_ptr = &xi;

    in_process_sequence_dll(buff, sizeof(buff), "");
    named_pipe_server nps(buff, &flog);
    named_pipe_server::register_named_pipe_server(&nps);
    gle = nps.start_listening();
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        displayErrorToCerr("watch_notify() named_pipe_server::start_listening()", gle, &flog);
        return gle;
    }
    if(opt_verbose) time_header_out(flog) << "watch_notify() named_pipe_server start" << endl;

#ifdef _DEBUG
    WaitForInputIdle(phproc->get_handle(), INFINITE);
    WaitForInputIdle(phproc_job->get_handle(), INFINITE);
#endif
    proc_list.push_back(phproc);
    proc_list.push_back(phproc_job);

    HANDLE hSema = CreateSemaphore(NULL, WORKER_CORE_NUM, WORKER_CORE_NUM, "Global\\semaphore_compress_process");
    if(hSema == NULL)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() CreateSemaphore()", gle, &flog);
        if(gle == ERROR_ALREADY_EXISTS)
        {
            time_header_out(flog) << "semaphore has existed, try to open it." << endl;
            hSema = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, "Global\\semaphore_compress_process");
            if(hSema == NULL) return displayErrorToCerr("watch_notify() OpenSemaphore()", GetLastError(), &flog);
        }
        else
            return gle;
    }
    
    HANDLE *pha = NULL;
    size_t file_in_compress_queue = 0;
    while(GetSignalInterruptValue() == 0)
    {
		// handle's waiting order: named pipe, dir monitor, compress proc, qr & job proc
		vector< BASE_HANDLE_PAIR > hs;
        hs.reserve(map_handle_context.size() + proc_list.size());
        transform(map_handle_context.begin(), map_handle_context.end(), back_inserter(hs), [](const HANDLE_PAIR &p) { return BASE_HANDLE_PAIR(p.first, p.second); });
		sort(hs.begin(), hs.end(), handle_less);
		vector< BASE_HANDLE_PAIR >::const_iterator it_null = find_if(hs.cbegin(), hs.cend(), [](const BASE_HANDLE_PAIR &p) { return p.second == NULL; });
		if(it_null != hs.cend()) hs.erase(it_null, hs.cend());

        // append proc handle to hs
        transform(proc_list.begin(), proc_list.end(), back_inserter(hs), [](handle_proc *p) { return BASE_HANDLE_PAIR(p->get_handle(), p); });

        if(debug_mode)
        {
            if(compress_queue.size() > file_in_compress_queue * 1.2)
            {
                file_in_compress_queue = compress_queue.size();
                time_header_out(flog) << "compress queue increase 20%: " << dec << file_in_compress_queue << endl;
                for_each(hs.cbegin(), hs.cend(), [&flog](const BASE_HANDLE_PAIR p) {
                    time_header_out(flog) << hex << setfill('0') << setw(8) << p.first << " ";
	                if(dynamic_cast<handle_compress*>(p.second))
                    {
                        handle_compress *phc = dynamic_cast<handle_compress*>(p.second);
                        flog << phc->get_notify_context().src_notify_filename << endl;
                    }
	                else
                    {
                        base_dir *phm = dynamic_cast<base_dir*>(p.second);
                        flog << phm->get_path() << " " << phm->get_meta_notify_filename() << endl;
                    }
                });
            }
            else if(compress_queue.size() < file_in_compress_queue * 0.8) file_in_compress_queue = compress_queue.size();
        }

        size_t hsize = 2 + hs.size();
        if(pha) delete[] pha;
        pha = new HANDLE[hsize];
        pha[0] = nps.get_handle(); // map_handle_context[named pipe listening handle] == NULL
        pha[1] = qrnps.get_handle();
        transform(hs.cbegin(), hs.cend(), &pha[2], [](const BASE_HANDLE_PAIR &p) { return p.first; });

        DWORD wr = WAIT_IO_COMPLETION;
		do {
			wr = WaitForMultipleObjectsEx(min(hsize, MAXIMUM_WAIT_OBJECTS), pha, FALSE, loop_wait, TRUE);
#ifdef _DEBUG
			if(wr == WAIT_IO_COMPLETION) time_header_out(cerr) << "WAIT_IO_COMPLETION : WaitForMultipleObjectsEx() again" << endl;
#endif
		} while(wr == WAIT_IO_COMPLETION);

        if(wr == WAIT_TIMEOUT || wr == WAIT_IO_COMPLETION)
        {
        }
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + min(hsize, MAXIMUM_WAIT_OBJECTS))
        {
            HANDLE waited = pha[wr - WAIT_OBJECT_0];
            handle_compress *phcompr = map_handle_context.count(waited) ? map_handle_context[waited] : NULL;
            
            if(waited == qrnps.get_handle()) // pipe client(dcmmkdir) connect incoming
            {
                gle = qrnps.pipe_client_connect_incoming();
                if(gle)
                {
                    displayErrorToCerr("watch_notify() named_pipe_listener::pipe_client_connect_incoming()", gle, &flog);
                    break;
                }
            }
            else if(waited == nps.get_handle()) // pipe client(dcmmkdir) connect incoming
            {
                gle = nps.pipe_client_connect_incoming();
                if(gle)
                {
                    displayErrorToCerr("watch_notify() named_pipe_server::pipe_client_connect_incoming()", gle, &flog);
                    break;
                }
            }
            else if(phcompr)
            {
                ReleaseSemaphore(hSema, 1, NULL);
                map_handle_context.erase(phcompr->get_handle());
                NOTIFY_FILE_CONTEXT nfc = phcompr->get_notify_context();
                time_header_out(flog) << "watch_notify() handle_compress exit: " << nfc.assoc.path << " " << nfc.file.filename << " " << nfc.file.unique_filename << endl;
                if(debug_mode) phcompr->print_state();
                compress_complete(phcompr->get_id(), nfc.file.studyUID, true, nps, nfc, flog);
                delete phcompr;
            }
            else
            {
                time_header_out(flog) << "watch_notify() phcompr is NULL, missing handle " << waited << endl;
                for(int i = 0; i < hsize; ++i) flog << pha[i] << ' '; flog << endl;
                break;
            }
        }
        else
        {
			gle = GetLastError();
			ostringstream errstrm;
			errstrm << "watch_notify() WaitForMultipleObjectsEx() return " << hex << wr << ", objs count is min(" << hsize << ", MAXIMUM_WAIT_OBJECTS = " << MAXIMUM_WAIT_OBJECTS << ")";
			for(size_t i = 0; i < min(hsize, MAXIMUM_WAIT_OBJECTS); ++i) errstrm << ", " << hex << pha[i];
            displayErrorToCerr(errstrm.str().c_str(), gle, &flog);
			continue;
        }

        // try compress queue
		DWORD dw = WAIT_OBJECT_0;
		NOTIFY_LIST::iterator it = compress_queue.begin();
        while(it != compress_queue.end() && dw == WAIT_OBJECT_0)
        {
			bool start_compress_process_ok = false;
			dw = WaitForSingleObject(hSema, 0);
			if(dw == WAIT_OBJECT_0)
			{
				set<string> exist_uniquename; // don't start same study/series/instance, avoid file lock
				for(HANDLE_MAP::const_iterator ite = map_handle_context.begin(); ite != map_handle_context.end(); ++ite)
				{
					if(ite->second) exist_uniquename.insert(ite->second->get_notify_context().file.unique_filename);
				}

                if(exist_uniquename.find(it->file.unique_filename) == exist_uniquename.end())
                {   // no same study/series/instance
                    NOTIFY_FILE_CONTEXT nfc = *it;
                    it = compress_queue.erase(it);

                    string received_instance_file_path(nfc.assoc.path);
                    received_instance_file_path.append(1, '\\').append(nfc.file.filename);
                    struct _stat64 fs;
                    if(_stat64(received_instance_file_path.c_str(), &fs))
                    {
                        char msg[256];
                        strerror_s(msg, errno);
                        time_header_out(flog) << __FUNCSIG__" _stat64(" << received_instance_file_path << ") fail: " << msg << endl;
                        fs.st_size = 0LL;

                        compress_complete(nfc.assoc.id, nfc.file.studyUID, false, nps, nfc, flog);
                        ReleaseSemaphore(hSema, 1, NULL);
                        continue;                           
                    }
                    nfc.file.file_size_receive = fs.st_size;

                    handle_compress* compr_ptr = handle_compress::make_handle_compress(nfc, flog);
                    if(compr_ptr)
                    {
						compr_ptr->set_priority(BELOW_NORMAL_PRIORITY_CLASS);
                        if(compr_ptr->start_process(false))
                        {   // failed
                            time_header_out(flog) << "watch_notify() handle_compress::start_process() failed: " << nfc.assoc.path << " " << nfc.file.filename << " " << nfc.file.unique_filename << endl;
                            delete compr_ptr;
                        }
                        else// succeed
                        {
                            time_header_out(flog) << "watch_notify() handle_compress::start_process() OK: " << nfc.assoc.path << " " << nfc.file.filename << " " << nfc.file.unique_filename << endl;
                            map_handle_context[compr_ptr->get_handle()] = compr_ptr;
                            start_compress_process_ok = true;
                        }
                    }
                    else
                        time_header_out(flog) << "watch_notify() handle_compress::make_handle_compress() failed: " << nfc.assoc.path << " " << nfc.file.filename << endl;
                    break; // exit for(NOTIFY_LIST::iterator it = compress_queue.begin()
                }
                else
                {
                    if(opt_verbose) time_header_out(flog) << "watch_notify() skip exist compress unique name " << it->file.unique_filename
                        << " src notify: " << it->src_notify_filename << endl;
                    ++it;
                }

				if(!start_compress_process_ok) ReleaseSemaphore(hSema, 1, NULL);
			}
			else if(dw == WAIT_FAILED)
                displayErrorToCerr("watch_notify() WaitForSingleObject(hSema)", GetLastError(), &flog);
#ifdef _DEBUG
			else if(dw == WAIT_TIMEOUT)
				time_header_out(flog) << "watch_notify() WaitForSingleObject(hSema) WAIT_TIMEOUT" << endl;
#endif
		} // end while(it != compress_queue.end() && dw == WAIT_OBJECT_0)

 		if(wr == WAIT_TIMEOUT)
		{
			// handle_dir pick up
			set<string> exist_association_paths;
			HANDLE_DIR_MAP::iterator it = handle_dir_map.begin();
			while(it != handle_dir_map.end())
			{
				handle_dir *phdir = NULL;
				if(it->second) phdir = it->second;
				if(phdir && phdir->get_id().length())
				{
					exist_association_paths.insert(exist_association_paths.begin(), phdir->get_path());
					if(close_handle_dir(phdir, NULL, nps, true, flog))
					{
						it = handle_dir_map.erase(it);
						continue;
					}
				}
				++it;
			}

			// write jdf to orders_notify
			set<string> exist_study_in_queue;
			transform(compress_queue.cbegin(), compress_queue.cend(), inserter(exist_study_in_queue, exist_study_in_queue.begin()),
				[](const NOTIFY_FILE_CONTEXT &nfc) { return string(nfc.study.studyUID); });

			nps.check_study_timeout_to_generate_jdf(exist_study_in_queue, exist_association_paths);

            //switch log file
            GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_service_n.txt", buff, sizeof(buff));
            if(flog.tellp() > 10 * 1024 * 1024 || strncmp(buff, current_log_path.c_str(), 20)) // 20 == strlen("pacs_log\\2016\\10\\10\\")
            {
	            if(PrepareFileDir(buff))
                {
                    if(flog.is_open())
                    {
                        time_header_out(flog) << "to be continued" << endl;
                        flog.close();
                    }
                    flog.open(buff);
                    if(flog.fail())
                    {
                        cerr << "watch_notify() switch log " << buff << " failed" << endl;
                        break; // while(GetSignalInterruptValue() == 0)
                    }
                    time_header_out(flog) << "continuation of " << current_log_path << endl;
                    current_log_path = buff;
                }
	            else
                {
                    cerr << "watch_notify() switch log failed, PrepareFileDir(" << buff << ") failed" << endl;
		            break; // while(GetSignalInterruptValue() == 0)
                }
            }
		}
    } // end while(GetSignalInterruptValue() == 0)

    if(GetSignalInterruptValue())
    {
        if(opt_verbose) time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;
    }
    else displayErrorToCerr("watch_notify() main loop exit", gle, &flog);

    if(hSema)
    {
        while(ReleaseSemaphore(hSema, 1, NULL)) ;
        CloseHandle(hSema);
        hSema = NULL;
    }
    
    // print association state
    for(HANDLE_DIR_MAP::iterator it = handle_dir_map.begin(); it != handle_dir_map.end(); ++it)
    {
        handle_dir *pha = it->second;
        if(debug_mode && pha) pha->print_state();
        delete it->second;
    }
    return gle;
}
