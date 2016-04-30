#include "stdafx.h"
#include "commonlib.h"

using namespace std;
using namespace handle_context;

static char buff[1024];

static HANDLE_MAP map_handle_context;
NOTIFY_LIST compress_queue;

static DWORD process_meta_notify_file(const string &notify_file, ostream &flog)
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
        string msg("process_meta_notify_file() open file ");
        msg.append(ntffn);
        return displayErrorToCerr(msg.c_str(), gle, &flog);
    }
    string cmd, path, assoc_id, calling, called, remote, port, transfer_syntax;
    DWORD tag, pid, gle = 0;
    ntff >> cmd >> hex >> tag >> path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port >> transfer_syntax;
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
    handle_dir *pclz_dir = new handle_dir(hdir, assoc_id, path);
    gle = pclz_dir->find_files(flog, [&flog, pclz_dir](const string &filename) { return pclz_dir->process_notify(filename, flog); });
    if(gle == 0) map_handle_context[hdir] = pclz_dir;
    else
    {
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
    handle_proc *phproc = new handle_proc("", buff, cmd, "dcmqrscp");
    DWORD gle = phproc->start_process(flog);
    if(gle)
    {
        displayErrorToCerr("watch_notify() handle_proc::create_process(dcmqrscp) at beginning", gle, &flog);
        return gle;
    }
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
    
    vector<HANDLE> ha;
    sprintf_s(buff, "%s\\pacs\\"NOTIFY_BASE, GetPacsBase());
    HANDLE hbase = FindFirstChangeNotification(buff, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hbase == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        displayErrorToCerr("watch_notify() FindFirstChangeNotification(store_notify)", gle, &flog);
        goto clean_child_proc;
    }
    handle_dir *pclz_base_dir = new handle_dir(hbase, "", NOTIFY_BASE);
    map_handle_context[hbase] = pclz_base_dir;
    
    // collect exist dfc files
    if(pclz_base_dir->find_files(flog, [&flog](const string& filename) { return process_meta_notify_file(filename, flog); }))
        goto clean_child_proc;

    while(GetSignalInterruptValue() == 0)
    {
        ha.clear();
        transform(map_handle_context.begin(), map_handle_context.end(), back_inserter(ha), 
            [](const HANDLE_PAIR &p) { return p.first; });

        DWORD wr = WaitForMultipleObjects(ha.size(), ha.data(), FALSE, 1000);

        if(wr == WAIT_TIMEOUT)
        {

        }
        else if(wr >= WAIT_OBJECT_0 && wr < WAIT_OBJECT_0 + map_handle_context.size())
        {
            HANDLE waited = ha[wr - WAIT_OBJECT_0];
            handle_compress *phcompr = NULL;
            handle_proc *phproc = NULL;
            handle_dir *phdir = NULL;
            notify_file *pb = map_handle_context[waited];
            if(pb)
            {
                if(phcompr = dynamic_cast<handle_compress*>(pb))
                {
                    ReleaseSemaphore(hSema, 1, NULL);
                    map_handle_context.erase(phcompr->get_handle());
                    CMOVE_NOTIFY_CONTEXT *pnc = phcompr->get_notify_context_ptr();
                    delete phcompr;
                    delete pnc; // todo: shall transfer ptr to dcmmkdir
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
                else if(phdir = dynamic_cast<handle_dir*>(pb))
                {
                    if(phdir->get_association_id().length()) // some file in storedir/association_id
                        gle = phdir->find_files(flog, [&flog, phdir](const string& filename) { return phdir->process_notify(filename, flog); });
                    else // new file in store_notify
                        gle = phdir->find_files(flog, [&flog](const string& filename) { return process_meta_notify_file(filename, flog); });
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
            DWORD dw = WAIT_OBJECT_0;
            while((dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED) && compress_queue.size() > 0)
            {
                dw = WaitForSingleObject(hSema, 0);
                if(dw == WAIT_OBJECT_0 || dw == WAIT_ABANDONED)
                {
                    CMOVE_NOTIFY_CONTEXT *pnc = compress_queue.front();
                    compress_queue.pop_front();
                    handle_compress* compr_ptr = handle_compress::make_handle_compress(pnc, map_handle_context);
                    if(compr_ptr)
                    {
                        if(compr_ptr->start_process(flog))
                        {
                            ReleaseSemaphore(hSema, 1, NULL);
                            delete compr_ptr;
                        }
                        else
                            map_handle_context[compr_ptr->get_handle()] = compr_ptr;
                    }
                    else
                        ReleaseSemaphore(hSema, 1, NULL);
                }
                else if(dw == WAIT_FAILED)
                    displayErrorToCerr("compress_queue_to_workers() WaitForSingleObject(hSema)", dw);
                // else WAIT_TIMEOUT, compress process is too much.
            }
        }
    }
    if(GetSignalInterruptValue() && opt_verbose)
        time_header_out(flog) << "watch_notify() WaitForMultipleObjects() get Ctrl-C" << endl;

clean_child_proc:
    if(hSema)
    {
        while(ReleaseSemaphore(hSema, 1, NULL)) ;
        CloseHandle(hSema);
        hSema = NULL;
    }
    for(HANDLE_MAP::iterator it = map_handle_context.begin(); it != map_handle_context.end(); ++it)
        delete it->second;
    return gle;
}
