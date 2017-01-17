#include "stdafx.h"

using namespace std;
using namespace handle_context;

file_notify& file_notify::operator=(const file_notify& r)
{
    base_dir::operator=(r);
    assoc_id = r.assoc_id;
    hash = r.hash;
    unique_filename = r.unique_filename;
    expected_xfer = r.expected_xfer;
    rec_file_size = r.rec_file_size;
    study_uid = r.study_uid;
    seq = r.seq;
    return *this;
}

void file_notify::clear()
{
    set_id("");
    set_path("");
    set_notify_filename("");
    assoc_id.clear(); hash.clear(); study_uid.clear();
    unique_filename.clear(); expected_xfer.clear(); auto_publish.clear();
    rec_file_size = 0LL;
    seq = 0;
}

void file_notify::print_state() const
{
    *pflog << "file_notify::print_state() instance_filename: " << get_id() << endl
        << "\tstudy_uid: " << study_uid << endl
        << "\tassoc_id: " << assoc_id << endl
        << "\texpected_xfer: " << expected_xfer << endl
        << "\tauto_publish: " << auto_publish << endl
        << "\tseq: " << seq << endl
        << "\thash: " << hash << endl
        << "\tunique_filename: " << unique_filename << endl
        << "\trec_file_size: " << rec_file_size << endl;
    base_dir::print_state();
}

file_notify::~file_notify()
{
    if(opt_verbose) time_header_out(*pflog) << "~file_notify():" << endl;
    if(opt_verbose) print_state();
    *pflog << "~file_notify() OK" << endl;
}

string relationship::get_assoc_id() const { return sp_assoc ? sp_assoc->get_id() : ""; }
string relationship::get_study_uid() const { return sp_study ? sp_study->get_id() : ""; }
relationship::relationship(shared_ptr<np_conn_assoc_dir> sp_assoc, shared_ptr<study_dir> sp_study, int time_out_diff, ostream *pflog)
    : base_dir("", "-", "-", time_out_diff, pflog), sp_assoc(sp_assoc), sp_study(sp_study), assoc_disconn(false)
{
    std::string id, empty_str;
    id.append(sp_assoc ? sp_assoc->get_id() : empty_str).append("<->").append(sp_study ? sp_study->get_id() : empty_str);
    set_id(id);
}

bool relationship::is_timeout_or_close()
{
    if(assoc_disconn) return true;
    assoc_disconn = (sp_assoc->is_disconn() || is_time_out());
    return assoc_disconn;
}

bool relationship::add_file_notify(const shared_ptr<file_notify> &sp_job)
{
    if(sp_job && sp_job->get_notify_filename().length())
    {
        file_queue[sp_job->get_notify_filename()] = sp_job;
        refresh_last_access();
        assoc_disconn = false;
        return true;
    }
    else return false;
}

shared_ptr<file_notify> relationship::find_file_notify(const string &notify_filename) const
{
    FILE_QUEUE::const_iterator it = file_queue.find(notify_filename);
    if(it != file_queue.cend()) return it->second;
    else return NULL;
}

FILE_QUEUE::const_iterator relationship::get_first_notify_filename_greater(const string &base) const
{
    return find_if(file_queue.cbegin(), file_queue.cend(), [&base](const FILE_QUEUE_PAIR &p) {
        return (p.second && p.second->get_notify_filename().compare(base) > 0); });
}

void relationship::erase(const string &notify_filename)
{
    if(opt_verbose) time_header_out(*pflog) << "relationship::erase(" << notify_filename << ")" << endl;
    file_queue.erase(notify_filename);
}

void relationship::print_state() const
{
    time_header_out(*pflog) << "relationship::print_state(): " << get_id() << endl
        << "\tfile_queue:" << endl;
    for_each(file_queue.cbegin(), file_queue.cend(), [](const FILE_QUEUE_PAIR &p) { if(p.second) p.second->print_state(); });
    *pflog << "\tindex_queue:" << endl;
    for_each(index_queue.cbegin(), index_queue.cend(), [](const FILE_QUEUE_PAIR &p) { if(p.second) p.second->print_state(); });
    *pflog << "relationship::print_state() OK." << endl;
}

relationship::~relationship()
{
    if(opt_verbose)
    {
        time_header_out(*pflog) << "~relationship():" << endl;
        print_state();
        *pflog << "~relationship() OK." << endl;
    }
}

void relationship::find_first_file_notify_to_start_process(HANDLE hSema, PROC_COMPR_LIST &proc_list, ostream &flog)
{
    char buff[MAX_PATH];
	DWORD dw = WAIT_OBJECT_0;
    shared_ptr<relationship> new_rela;
    shared_ptr<file_notify> new_file_notify;
    string current_notify_filename_base;
    do
    {
        new_rela = NULL;
        new_file_notify = NULL;
        RELA_POS_PAIR pr;
        if(proc_list.size() < WORKER_CORE_NUM) // some core idle
            pr = study_dir::find_first_job_in_studies(current_notify_filename_base);
        if(pr.first == NULL || pr.second == pr.first->get_file_queue_cend()) break; // no file_notify in queue
        new_rela = pr.first;
        new_file_notify = pr.second->second;
        if(new_file_notify == NULL) { pr.first->erase(pr.second->first); continue; } // wrong file_notify, erase it, next loop

        const string &unique_filename = new_file_notify->get_unique_filename();
        PROC_COMPR_LIST::iterator ite = find_if(proc_list.begin(), proc_list.end(),
            [&unique_filename](const handle_compress *p) { return (p->get_id().compare(unique_filename) == 0); });

        if(ite == proc_list.end()) // no same instance is in compressing
        {
            pr.first->erase(pr.second->first);

            string received_instance_file_path(GetPacsTemp());
            received_instance_file_path.append("\\pacs\\").append(new_file_notify->get_path()).append(1, '\\').append(new_file_notify->get_instance_filename());
            struct _stat64 fs;
            if(_stat64(received_instance_file_path.c_str(), &fs))
            {
                strerror_s(buff, errno);
                time_header_out(flog) << __FUNCSIG__" _stat64(" << received_instance_file_path << ") fail: " << buff << endl;
                fs.st_size = 0LL;
                continue;
            }

			dw = WaitForSingleObject(hSema, 0);
            if(dw == WAIT_OBJECT_0)
            {
                new_file_notify->set_rec_file_size(fs.st_size);
                handle_compress* compr_ptr = handle_compress::make_handle_compress(pr.first->get_study_uid(), new_rela, new_file_notify, flog);
                if(compr_ptr)
                {
					compr_ptr->set_priority(BELOW_NORMAL_PRIORITY_CLASS);
                    if(compr_ptr->start_process(false))
                    {   // failed
                        time_header_out(flog) << "watch_notify() handle_compress::start_process() failed: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << " " << new_file_notify->get_unique_filename() << endl;
                        delete compr_ptr;
                        ReleaseSemaphore(hSema, 1, NULL);
                    }
                    else// succeed
                    {
                        time_header_out(flog) << "watch_notify() handle_compress::start_process() OK: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << " " << new_file_notify->get_unique_filename() << endl;
                        proc_list.push_back(compr_ptr);
                    }
                }
                else
                {
                    time_header_out(flog) << "watch_notify() handle_compress::make_handle_compress() failed: " << new_file_notify->get_path() << " " << new_file_notify->get_instance_filename() << endl;
                    ReleaseSemaphore(hSema, 1, NULL);
                }
            }
			else if(dw == WAIT_FAILED)
            {
                displayErrorToCerr("watch_notify() WaitForSingleObject(hSema)", GetLastError(), &flog);
                break; // break to main loop
            }
#ifdef _DEBUG
			else if(dw == WAIT_TIMEOUT)
            {
				time_header_out(flog) << "watch_notify() WaitForSingleObject(hSema) WAIT_TIMEOUT" << endl;
                break; // break to main loop
            }
#endif
        }
        else // the same instance is in compressing
        {
            time_header_out(flog) << "watch_notify() skip exist compress unique name "
                << new_file_notify->get_unique_filename() << " src notify: " << new_file_notify->get_notify_filename() << endl;
            current_notify_filename_base = new_file_notify->get_notify_filename();
        }
    } while(new_file_notify);
}
