#include "stdafx.h"
#include "commonlib.h"

using namespace std;
using namespace handle_context;

static char buff[FILE_BUF_SIZE];

static HANDLE_MAP map_handle_context;
static NOTIFY_LIST compress_queue;

static bool select_handle_dir_by_association_path(const meta_notify_file* pnf, const string &association_id, const string &path, ostream &flog)
{
    if(pnf == NULL) return false;
    bool same_assoc = (association_id.compare(pnf->get_association_id()) == 0);
    bool same_path = (path.compare(pnf->get_path()) == 0);

    if(same_assoc && same_path)
        return (NULL != dynamic_cast<const handle_dir*>(pnf));
    else if(same_assoc || same_path)
    {
        time_header_out(flog) << "select_handle_dir_by_association_path(" << association_id << ", " << path 
            << ") mismatch (" << pnf->get_association_id() << ", " << pnf->get_path() << ")" << endl;
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

static DWORD process_meta_notify_file(handle_dir *base_dir, const string &notify_file, ostream &flog)
{
    string ntffn(NOTIFY_BASE);
    ntffn.append(1, '\\').append(notify_file);
    string meta_notify_file_txt(ntffn);
    string::size_type pos = meta_notify_file_txt.rfind('.');
    meta_notify_file_txt.erase(pos).append(".txt");
    // if .txt exist, meta notify file has been processed.
    if(0 == _access(meta_notify_file_txt.c_str(), 0))
        return ERROR_DATATYPE_MISMATCH; // skip it

    if(opt_verbose) time_header_out(flog) << "process_meta_notify_file() receive association notify " << notify_file << endl;
#ifdef _DEBUG
    time_header_out(cerr) << "process_meta_notify_file() receive association notify " << notify_file << endl;
#endif
    ifstream ntff(ntffn, ios_base::in, _SH_DENYWR);
    if(ntff.fail())
    {
        DWORD gle = GetLastError();
        string msg("process_meta_notify_file() open file ");
        msg.append(ntffn);
        return displayErrorToCerr(msg.c_str(), gle, &flog);
    }
    
    string cmd, path, assoc_id, calling, called, remote, port, transfer_syntax, auto_publish;
    DWORD tag, pid, gle = 0;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port >> transfer_syntax >> auto_publish;
    if(ntff.is_open()) ntff.close();
#ifdef _DEBUG
    time_header_out(cerr) << cmd << " " << hex << tag << " " << path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;
#endif
    if(path[1] != ':') path.insert(0, "\\pacs\\").insert(0, GetPacsBase());
    pos = path.length();
    path.append("\\state");
    
    HANDLE hdir = FindFirstChangeNotification(path.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hdir == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        sprintf_s(buff, "process_meta_notify_file() FindFirstChangeNotification(%s)", path.c_str());
        write_association_complete_text_file(meta_notify_file_txt, buff, gle, flog);
        return displayErrorToCerr(buff, gle, &flog);
    }

    // create association(handle_dir) instance
    path.erase(pos);
    // path is association base dir
    const HANDLE_MAP::iterator it = find_if(map_handle_context.begin(), map_handle_context.end(),
        [&assoc_id, &path, &flog](const HANDLE_PAIR &p) { return select_handle_dir_by_association_path(p.second, assoc_id, path, flog); });
    if(it != map_handle_context.end())
    {
        sprintf_s(buff, "association id %s : %s exist.", assoc_id.c_str(), path.c_str());
        write_association_complete_text_file(meta_notify_file_txt, buff, ERROR_FILE_EXISTS, flog);
        time_header_out(flog) << "process_meta_notify_file() " << buff << endl;
        return ERROR_DATATYPE_MISMATCH;
    }

    handle_dir *pclz_dir = new handle_dir(hdir, assoc_id, path, notify_file, &flog);
    gle = pclz_dir->find_files(flog, [&flog, pclz_dir](const string &filename) { return pclz_dir->process_notify(filename, compress_queue, flog); });
    if(gle == 0)
    {
        map_handle_context[hdir] = pclz_dir;
        if(opt_verbose) time_header_out(flog) << "process_meta_notify_file() add association " << pclz_dir->get_association_id() << " " << pclz_dir->get_path() << endl;
        return 0;
    }
    else
    {   // discard the meta notify
        delete pclz_dir;
        write_association_complete_text_file(meta_notify_file_txt, "process_meta_notify_file() handle_dir::find_files()", gle, flog);
        return displayErrorToCerr("process_meta_notify_file() handle_dir::find_files()", gle, &flog);
    }
}

static void compress_complete(const string &assoc_id, const string &study_uid, bool compress_ok,
    named_pipe_server &nps, NOTIFY_FILE_CONTEXT &nfc, ostream &flog)
{
    if(opt_verbose) time_header_out(flog) << "compress_complete() dcmcjpeg " << (compress_ok ? "complete" : "failed")
        << ", find association " << assoc_id << " to set complete." << endl;
    handle_dir *phd = NULL;
	for(HANDLE_MAP::const_iterator it = map_handle_context.cbegin(); it != map_handle_context.cend(); ++it)
	{
        if(it->second == NULL) continue;
        if(0 == assoc_id.compare(it->second->get_association_id()))
        {
            phd = dynamic_cast<handle_dir*>(it->second);
            if(phd) break;
        }
	}
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
    if(phdir && phdir->get_association_id().length())
    {
        for(HANDLE_MAP::iterator it = map_handle_context.begin(); it != map_handle_context.end(); ++it)
        {
            handle_compress *phcompr = NULL;
            if(it->second) phcompr = dynamic_cast<handle_compress*>(it->second);
            if(phcompr && phcompr->get_association_id() == phdir->get_association_id()) return false;
        }

        for(NOTIFY_LIST::iterator it = compress_queue.begin(); it != compress_queue.end(); ++it)
        {
            if(phdir->get_association_id().compare(it->assoc.id) == 0) return false;
        }

        if(phdir->is_association_disconnect() || phdir->is_time_out())
        {
            if(opt_verbose)
            {
                time_header_out(flog) << "close_handle_dir()" << (pick_up ? " pick up" : "") << " association " << phdir->get_association_id() << " complete, erease from map_handle_context." << endl;
                flog << "\t" << phdir->close_description() << " close." << endl;
            }
            // close monitor handle, all_compress_ok_notify is not processed.
            phdir->send_all_compress_ok_notify_and_close_handle();
            phdir->broadcast_assoc_close_action_to_all_study(nps);
                            
            pclz_base_dir->remove_file_from_list(phdir->get_meta_notify_filename());

            if(opt_verbose || phdir->file_complete_remain())
            {
                time_header_out(flog) << "close_handle_dir() handle_dir" << (pick_up ? " pick up" : "") << " exit:" << endl;
                if(debug_mode) phdir->print_state();
            }
            delete phdir;

            return true;
        }
    }
    return false;
}

static bool handle_less(const HANDLE_PAIR &p1, const HANDLE_PAIR &p2)
{
	if(p1.second == NULL && p2.second == NULL) return p1.first < p2.first;
	else if(p2.second == NULL) return true;
	else if(p1.second == NULL) return false;

	int c1 = 3, c2 = 3;

	if(dynamic_cast<handle_compress*>(p1.second)) c1 = 2;
	else if(dynamic_cast<handle_dir*>(p1.second)) c1 = 1;
	else c1 = 3; // qr or job

	if(dynamic_cast<handle_compress*>(p2.second)) c2 = 2;
	else if(dynamic_cast<handle_dir*>(p2.second)) c2 = 1;
	else c2 = 3; // qr or job

	if(c1 == c2)
    {
        if(c1 == 1) // p1 and p2 is handle_dir
        {
            if(p1.second->get_association_id().length() == 0 && p2.second->get_association_id().length() > 0) return true;
            else if(p2.second->get_association_id().length() == 0 && p1.second->get_association_id().length() > 0) return false;
            // else (p1 and p2 length > 0) || (p1 and p2 length == 0), compare last_access
        }
        return p1.second->get_last_access() < p2.second->get_last_access();
    }
	return c1 < c2;
}

static const string debug_mode_header("DebugMode");

int watch_notify(string &cmd, ostream &flog)
{
    sprintf_s(buff, "%s\\pacs\\store_notify\\*.dfc", GetPacsBase());
    disable_remained_meta_notify_file(buff, flog);
    sprintf_s(buff, "%s\\orders_study\\*.ini", GetPacsBase());
    disable_remained_meta_notify_file(buff, flog);

    // start dcmqrscp.exe parent proc
    sprintf_s(buff, "%s\\pacs", GetPacsBase());
    handle_proc *phproc = new handle_proc("", buff, cmd, "dcmqrscp", &flog);
    DWORD gle = phproc->start_process(true);
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
    handle_proc *phproc_job = new handle_proc("", buff, cmd, "jobloader", &flog);
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
    map_handle_context[phproc->get_handle()] = phproc;
    map_handle_context[phproc_job->get_handle()] = phproc_job;

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
            if(hSema == NULL) return displayErrorToCerr("watch_notify() OpenSemaphore()", GetLastError(), &flog);
        }
        else
            return gle;
    }
    
    sprintf_s(buff, "%s\\pacs\\"NOTIFY_BASE, GetPacsBase());
    HANDLE hbase = FindFirstChangeNotification(buff, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hbase == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() FindFirstChangeNotification(store_notify)", gle, &flog);
        goto clean_child_proc;
    }
    handle_dir *pclz_base_dir = new handle_dir(hbase, "", NOTIFY_BASE, "", &flog);
    map_handle_context[hbase] = pclz_base_dir;
    
    // collect exist dfc files
    if(pclz_base_dir->find_files(flog, [pclz_base_dir, &flog](const string& filename) { return process_meta_notify_file(pclz_base_dir, filename, flog); }))
        goto clean_child_proc;

    HANDLE *pha = NULL;
    while(GetSignalInterruptValue() == 0)
    {
		// handle's waiting order: named pipe, dir monitor, compress proc, qr & job proc
		vector<HANDLE_PAIR> hs;
		hs.reserve(map_handle_context.size());
        transform(map_handle_context.begin(), map_handle_context.end(), back_inserter(hs), [](const HANDLE_PAIR &p) { return p; });
		sort(hs.begin(), hs.end(), handle_less);
		vector<HANDLE_PAIR>::const_iterator it_null = find_if(hs.cbegin(), hs.cend(), [](const HANDLE_PAIR &p) { return p.second == NULL; });
		if(it_null != hs.cend()) hs.erase(it_null, hs.cend());

        size_t hsize = hs.size() + 1;
        if(pha) delete[] pha;
        pha = new HANDLE[hsize];
        pha[0] = nps.get_handle(); // map_handle_context[named pipe listening handle] == NULL
        transform(hs.cbegin(), hs.cend(), &pha[1], [](const HANDLE_PAIR &p) { return p.first; });

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
            handle_compress *phcompr = NULL;
            handle_proc *phproc = NULL;
            handle_dir *phdir = NULL;
            meta_notify_file *pb = map_handle_context.count(waited) ? map_handle_context[waited] : NULL;
            
            if(waited == nps.get_handle()) // pipe client(dcmmkdir) connect incoming
            {
                gle = nps.pipe_client_connect_incoming();
                if(gle)
                {
                    displayErrorToCerr("watch_notify() named_pipe_server::pipe_client_connect_incoming()", gle, &flog);
                    break;
                }
            }
            else if(pb)
            {
                if(phcompr = dynamic_cast<handle_compress*>(pb))
                {
                    ReleaseSemaphore(hSema, 1, NULL);
                    map_handle_context.erase(phcompr->get_handle());
                    NOTIFY_FILE_CONTEXT nfc = phcompr->get_notify_context();
                    if(opt_verbose)
                    {
                        time_header_out(flog) << "watch_notify() compress proc exit:" << endl;
                        if(debug_mode) phcompr->print_state();
                    }
                    compress_complete(phcompr->get_association_id(), nfc.file.studyUID, true, nps, nfc, flog);
                    delete phcompr;
                }
                else if(phdir = dynamic_cast<handle_dir*>(pb))
                {
                    if(phdir->get_association_id().length()) // some file in storedir/association_id
                        gle = phdir->find_files(flog, [&flog, phdir](const string& filename) { return phdir->process_notify(filename, compress_queue, flog); });
                    else // new file in meta notify dir(store_notify)
                        gle = phdir->find_files(flog, [&flog, phdir](const string& filename) { return process_meta_notify_file(phdir, filename, flog); });
                }
                else if(phproc = dynamic_cast<handle_proc*>(pb))
                {
                    time_header_out(flog) << "watch_notify() " << phproc->get_exec_name() << " encounter error, exit." << endl;
                    map_handle_context.erase(waited);
                    goto clean_child_proc;
                    // restart proc?
                }
                else
                {
                    time_header_out(flog) << "unexcepting notify file " << pb->get_association_id() << ", " << pb->get_path() << endl;
                }
            }
            else
            {
                time_header_out(flog) << "watch_notify() missing handle " << waited << endl;
                map_handle_context.erase(waited);
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
					handle_compress *phc = NULL;
					if(phc = dynamic_cast<handle_compress*>(ite->second))
						exist_uniquename.insert(phc->get_notify_context().file.unique_filename);
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
                        continue;                           
                    }
                    nfc.file.file_size_receive = fs.st_size;

                    handle_compress* compr_ptr = handle_compress::make_handle_compress(nfc, flog);
                    if(compr_ptr)
                    {
						compr_ptr->set_priority(IDLE_PRIORITY_CLASS);
                        if(compr_ptr->start_process(false))
                        {   // failed
                            time_header_out(flog) << "watch_notify() handle_compress::start_process() failed: " << nfc.src_notify_filename << " " << nfc.file.filename << endl;
                            delete compr_ptr;
                        }
                        else// succeed
                        {
                            map_handle_context[compr_ptr->get_handle()] = compr_ptr;
                            start_compress_process_ok = true;
                        }
                    }
                    else
                        time_header_out(flog) << "watch_notify() handle_compress::make_handle_compress() failed: " << nfc.src_notify_filename << " " << nfc.file.filename << endl;
                    break; // exit for(NOTIFY_LIST::iterator it = compress_queue.begin()
                }
                else ++it;

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
			HANDLE_MAP::iterator it = map_handle_context.begin();
			while(it != map_handle_context.end())
			{
				handle_dir *phdir = NULL;
				if(it->second) phdir = dynamic_cast<handle_dir*>(it->second);
				if(phdir && phdir->get_association_id().length())
				{
					exist_association_paths.insert(exist_association_paths.begin(), phdir->get_path());
					if(close_handle_dir(phdir, pclz_base_dir, nps, true, flog))
					{
						it = map_handle_context.erase(it);
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
		}
    } // end while(GetSignalInterruptValue() == 0)

    if(GetSignalInterruptValue())
    {
        if(opt_verbose) time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;
    }
    else displayErrorToCerr("watch_notify() main loop exit", gle, &flog);

clean_child_proc:
    if(hSema)
    {
        while(ReleaseSemaphore(hSema, 1, NULL)) ;
        CloseHandle(hSema);
        hSema = NULL;
    }
    
    // print association state
    for(HANDLE_MAP::iterator it = map_handle_context.begin(); it != map_handle_context.end(); ++it)
    {
        handle_dir *pha = dynamic_cast<handle_dir*>(it->second);
        if(debug_mode && pha) pha->print_state();
        delete it->second;
    }
    return gle;
}
