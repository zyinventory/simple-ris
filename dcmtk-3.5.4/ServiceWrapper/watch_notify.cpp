#include "stdafx.h"
#include "commonlib.h"

using namespace std;
using namespace handle_context;

static char buff[FILE_BUF_SIZE];

static HANDLE_MAP map_handle_context;
NOTIFY_LIST compress_queue;

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

static DWORD process_meta_notify_file(handle_dir *base_dir, const string &notify_file, ostream &flog)
{
    string ntffn(NOTIFY_BASE);
    ntffn.append(1, '\\').append(notify_file);
    if(opt_verbose) time_header_out(flog) << "process_meta_notify_file() receive association notify " << notify_file << endl;
#ifdef _DEBUG
    time_header_out(cerr) << "process_meta_notify_file() receive association notify " << notify_file << endl;
#endif
    ifstream ntff(ntffn, ios_base::in, _SH_DENYRW);
    if(ntff.fail())
    {
        DWORD gle = GetLastError();
        if(gle != ERROR_SHARING_VIOLATION) base_dir->insert_complete(notify_file);
        string msg("process_meta_notify_file() open file ");
        msg.append(ntffn);
        return displayErrorToCerr(msg.c_str(), gle, &flog);
    }
    base_dir->insert_complete(notify_file); // if(gle != ERROR_SHARING_VIOLATION) find_files() must insert complete

    string cmd, path, assoc_id, calling, called, remote, port, transfer_syntax, auto_publish;
    DWORD tag, pid, gle = 0;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port >> transfer_syntax >> auto_publish;
    if(ntff.is_open()) ntff.close();
#ifdef _DEBUG
    time_header_out(cerr) << cmd << " " << hex << tag << " " << path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;
#endif
    if(path[1] != ':') path.insert(0, "\\pacs\\").insert(0, GetPacsBase());
    size_t pos = path.length();
    path.append("\\state");
    
    HANDLE hdir = FindFirstChangeNotification(path.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hdir == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        sprintf_s(buff, "process_meta_notify_file() FindFirstChangeNotification(%s)", path.c_str());
        return displayErrorToCerr(buff, gle, &flog);
    }

    // create association(handle_dir) instance
    path.erase(pos);
    // path is association base dir
    const HANDLE_MAP::iterator it = find_if(map_handle_context.begin(), map_handle_context.end(),
        [&assoc_id, &path, &flog](const HANDLE_PAIR &p) { return select_handle_dir_by_association_path(p.second, assoc_id, path, flog); });
    if(it != map_handle_context.end()) return 0;

    handle_dir *pclz_dir = new handle_dir(hdir, assoc_id, path, notify_file, &flog);
    gle = pclz_dir->find_files(flog, [&flog, pclz_dir](const string &filename) { return pclz_dir->process_notify(filename, flog); });
    if(gle == 0)
    {
        map_handle_context[hdir] = pclz_dir;
        if(opt_verbose) time_header_out(flog) << "process_meta_notify_file() add association " << pclz_dir->get_association_id() << " " << pclz_dir->get_path() << endl;
    }
    else
    {   // discard the meta notify
        delete pclz_dir;
        displayErrorToCerr("process_meta_notify_file() handle_dir::find_files()", gle, &flog);
        string badname(ntffn);
        badname.append(".bad");
        rename(ntffn.c_str(), badname.c_str());
    }
    return 0;
}

int watch_notify(string &cmd, ostream &flog)
{
    // start dcmqrscp.exe parent proc
    sprintf_s(buff, "%s\\pacs", GetPacsBase());
    handle_proc *phproc = new handle_proc("", buff, cmd, "dcmqrscp", &flog);
    DWORD gle = phproc->start_process(flog);
    if(gle)
    {
        displayErrorToCerr("watch_notify() handle_proc::create_process(dcmqrscp) at beginning", gle, &flog);
        return gle;
    }
    if(opt_verbose) time_header_out(flog) << "watch_notify() dcmqrscp start" << endl;
    
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
#endif
    map_handle_context[phproc->get_handle()] = phproc;

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
        size_t hsize = map_handle_context.size() + 1;
        if(pha) delete[] pha;
        pha = new HANDLE[hsize];
        transform(map_handle_context.begin(), map_handle_context.end(), pha, 
            [](const HANDLE_PAIR &p) { return p.first; });

        pha[hsize - 1] = nps.get_handle(); // map_handle_context[named pipe listening handle] == NULL

        DWORD wr = WaitForMultipleObjectsEx(hsize, pha, FALSE, 1000, TRUE);

        if(wr == WAIT_TIMEOUT)
        {
            nps.check_study_timeout_to_generate_jdf();
        }
        else if(wr == WAIT_IO_COMPLETION)
        {
            ;
        }
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + hsize)
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

                    string study_uid(nfc.file.studyUID), assoc_path(phcompr->get_path()), assoc_id(phcompr->get_association_id());
                    if(opt_verbose)
                    {
                        time_header_out(flog) << "watch_notify() compress proc exit:" << endl;
                        phcompr->print_state();
                    }
                    delete phcompr;
                    
                    if(opt_verbose) time_header_out(flog) << "watch_notify() dcmcjpeg complete, find association " << assoc_id << " to set complete." << endl;
                    handle_dir *phd = NULL;
                    const HANDLE_MAP::iterator it_assoc = find_if(map_handle_context.begin(), map_handle_context.end(),
                        [&assoc_id, &phd](const HANDLE_PAIR &p) -> bool {
                            if(p.second == NULL) return false;
                            if(0 == assoc_id.compare(p.second->get_association_id()))
                            {
                                phd = dynamic_cast<handle_dir*>(p.second);
                                if(phd) return true;
                            }
                            return false;
                        });

                    if(phd)
                    {
                        handle_study *phs = nps.find_handle_study(study_uid);
                        if(phs == NULL)
                            time_header_out(flog) << "watch_notify() src file " << nfc.src_notify_filename << " can't send action to dicomdir: missing study " << study_uid << endl;
                        phd->send_compress_complete_notify(nfc, phs, flog); // phs->append_action(action_from_association(nfc)); 
                    }
                    else time_header_out(flog) << "watch_notify() set src file " << nfc.src_notify_filename << " complete failed: missing association " << assoc_id << endl;
                }
                else if(phdir = dynamic_cast<handle_dir*>(pb))
                {
                    if(phdir->get_association_id().length()) // some file in storedir/association_id
                    {
                        gle = phdir->find_files(flog, [&flog, phdir](const string& filename) { return phdir->process_notify(filename, flog); });
                        if(phdir->is_association_disconnect() && phdir->file_complete_remain() == 0)
                        {
                            map_handle_context.erase(waited);
                            if(opt_verbose) time_header_out(flog) << "watch_notify() association " << phdir->get_association_id() << " complete, erease from map_handle_context." << endl;
                            // close monitor handle, all_compress_ok_notify is a comment.
                            phdir->send_all_compress_ok_notify_and_close_handle(flog);
                            phdir->broadcast_action_to_all_study(nps);
                            if(opt_verbose)
                            {
                                time_header_out(flog) << "watch_notify() handle_dir exit:" << endl;
                                phdir->print_state();
                            }
                            delete phdir;

                            // todo: if cmove assoc, start batch burning function in another dir.
                        }
                    }
                    else // new file in meta notify dir(store_notify)
                        gle = phdir->find_files(flog, [&flog, phdir](const string& filename) { return process_meta_notify_file(phdir, filename, flog); });
                }
                else if(phproc = dynamic_cast<handle_proc*>(pb))
                {
                    time_header_out(flog) << "watch_notify() dcmqrscp encounter error, restart." << endl;
                    gle = phproc->start_process(flog);
                    if(gle)
                    {
                        displayErrorToCerr("watch_notify() handle_proc::create_process(dcmqrscp) restart", gle, &flog);
                        goto clean_child_proc;
                    }
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
            gle = displayErrorToCerr("watch_notify() WaitForMultipleObjects()", GetLastError(), &flog);
            break;
        }

        // try compress queue
        if(compress_queue.size() > 0)
        {
            set<string> exist_uniquename; // don't start same study/series/instance, avoid file lock
            for(HANDLE_MAP::const_iterator it = map_handle_context.begin(); it != map_handle_context.end(); ++it)
            {
                handle_compress *phc = NULL;
                if(phc = dynamic_cast<handle_compress*>(it->second))
                    exist_uniquename.insert(phc->get_notify_context().file.unique_filename);
            }

            DWORD dw = WAIT_OBJECT_0;
            while((dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED) && compress_queue.size() > 0)
            {
                dw = WaitForSingleObject(hSema, 0);
                if(dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED)
                {
                    bool find_job = false, start_compress_process_ok = false;
                    for(NOTIFY_LIST::iterator it = compress_queue.begin(); it != compress_queue.end(); ++it)
                    {
                        if(exist_uniquename.find(it->file.unique_filename) == exist_uniquename.end())
                        {   // no same study/series/instance
                            find_job = true;
                            NOTIFY_FILE_CONTEXT nfc = *it;
                            compress_queue.erase(it);

                            handle_compress* compr_ptr = handle_compress::make_handle_compress(nfc, flog);
                            if(compr_ptr)
                            {
                                if(compr_ptr->start_process(flog))
                                {   // failed
                                    time_header_out(flog) << "watch_notify() handle_compress::start_process() failed: " << nfc.src_notify_filename << " " << nfc.file.filename << endl;
                                    delete compr_ptr;
                                }
                                else// succeed
                                {
                                    map_handle_context[compr_ptr->get_handle()] = compr_ptr;
                                    exist_uniquename.insert(nfc.file.unique_filename);
                                    start_compress_process_ok = true;
                                }
                            }
                            else
                                time_header_out(flog) << "watch_notify() handle_compress::make_handle_compress() failed: " << nfc.src_notify_filename << " " << nfc.file.filename << endl;
                            break; // exit for(NOTIFY_LIST::iterator it = compress_queue.begin()
                        }
                    }
                    if(!start_compress_process_ok) ReleaseSemaphore(hSema, 1, NULL);
                    if(!find_job)
                    {
                        if(compress_queue.size())
                            time_header_out(flog) << "watch_notify() can't find compress job to start process, compress_queue.size() is " << compress_queue.size() << endl;
                        break; // exit while((dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED) && compress_queue.size() > 0)
                    }
                }
                else if(dw == WAIT_FAILED)
                    displayErrorToCerr("compress_queue_to_workers() WaitForSingleObject(hSema)", dw, &flog);
                // else WAIT_TIMEOUT, compress process is too much.
            }
        }
    }
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
        if(pha) pha->print_state();
        delete it->second;
    }
    return gle;
}
