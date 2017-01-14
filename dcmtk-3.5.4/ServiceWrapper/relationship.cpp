#include "stdafx.h"

using namespace std;
using namespace handle_context;

file_notify& file_notify::operator=(const file_notify& r)
{
    base_path::operator=(r);
    assoc_id = r.assoc_id;
    notify_filename = r.notify_filename;
    hash = r.hash;
    unique_filename = r.unique_filename;
    instance_filename = r.instance_filename;
    expected_xfer = r.expected_xfer;
    rec_file_size = r.rec_file_size;
    study_uid = r.study_uid;
    seq = r.seq;
    return *this;
}

void file_notify::clear()
{
    set_path("");
    assoc_id.clear(); notify_filename.clear(); hash.clear(); study_uid.clear();
    unique_filename.clear(); instance_filename.clear(); expected_xfer.clear(); auto_publish.clear();
    rec_file_size = 0LL;
    seq = 0;
}

void file_notify::print_state() const
{
    *pflog << "file_notify::print_state() notify_filename: " << notify_filename << endl
        << "\tstudy_uid: " << study_uid << endl
        << "\tassoc_id: " << assoc_id << endl
        << "\texpected_xfer: " << expected_xfer << endl
        << "\tauto_publish: " << auto_publish << endl
        << "\tseq: " << seq << endl
        << "\tinstance_filename: " << instance_filename << endl
        << "\thash: " << hash << endl
        << "\tunique_filename: " << unique_filename << endl
        << "\trec_file_size: " << rec_file_size << endl;
    base_path::print_state();
}

file_notify::~file_notify()
{
    if(opt_verbose) time_header_out(*pflog) << "~file_notify():" << endl;
    if(opt_verbose) print_state();
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
}

relationship::~relationship()
{
    if(opt_verbose) time_header_out(*pflog) << "relationship::~relationship():" << endl;
    if(opt_verbose) print_state();
}
