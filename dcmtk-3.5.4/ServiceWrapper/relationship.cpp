#include "stdafx.h"

using namespace std;
using namespace handle_context;

file_notify& file_notify::operator=(const file_notify& r)
{
    assoc_id = r.assoc_id;
    path = r.path;
    notify_filename = r.notify_filename;
    hash = r.hash;
    unique_filename = r.unique_filename;
    instance_filename = r.instance_filename;
    expected_xfer = r.expected_xfer;
    rec_file_size = r.rec_file_size;
    study_uid = r.study_uid;
    seq = r.seq;
    pflog = r.pflog;
    return *this;
}

void file_notify::clear()
{
    assoc_id.clear(); path.clear(); notify_filename.clear(); hash.clear(); study_uid.clear();
    unique_filename.clear(); instance_filename.clear(); expected_xfer.clear();
    rec_file_size = 0LL;
    seq = 0;
}

file_notify::~file_notify()
{
    time_header_out(*pflog) << "~file_notify:" << std::endl << "\tstudy_uid: " << study_uid << std::endl
        << "\thash: " << hash << std::endl << "\tunique_filename: " << unique_filename << std::endl
        << "\tinstance_filename: " << instance_filename << std::endl << "\tassoc_id: " << assoc_id << std::endl
        << "\tpath: " << path << std::endl << "\tnotify_filename: " << notify_filename << std::endl
        << "\texpected_xfer: " << expected_xfer << std::endl << "\tseq: " << seq << std::endl
        << "\trec_file_size: " << rec_file_size << std::endl;
}

string relationship::get_id() const
{
    std::string id;
    return id.append(sp_assoc ? sp_assoc->get_id() : "").append(1, ':').append(sp_study ? sp_study->get_id() : "");
}

bool relationship::add_file(const shared_ptr<file_notify> &sp_job)
{
    if(sp_job && sp_job->get_notify_filename().length())
    {
        file_queue[sp_job->get_notify_filename()] = sp_job;
        return true;
    }
    else return false;
}
