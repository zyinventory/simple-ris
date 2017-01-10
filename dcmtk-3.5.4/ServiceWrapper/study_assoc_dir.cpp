#include "stdafx.h"

using namespace std;
using namespace handle_context;

string study_assoc_dir::empty_notify_filename;
STUDY_MAP study_assoc_dir::studies_map;

void study_assoc_dir::print_state() const
{
    *pflog << "study_assoc_dir::print_state() id: " << get_id() << endl
        << "\tassociations:" << endl;
    for(STR_SHARED_CONN_MAP::const_iterator it = associations.cbegin(); it != associations.cend(); ++it)
        *pflog << "\t\t" << (it->second == NULL ? "NULL" : it->second->get_id()) << endl;
    *pflog << "\tcompress_queue:" << endl;
    for(list<shared_ptr<compress_job> >::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
        *pflog << "\t\t" << (*it)->get_path() << "\\" << (*it)->get_instance_filename() << "(" << (*it)->get_notify_filename() << ") -> " << (*it)->get_hash() << "\\*\\" << (*it)->get_unique_filename() << endl;
    base_dir::print_state();
}

study_assoc_dir* study_assoc_dir::create_instance(const char *study_uid, const char *path, const char *meta_notify_file, ostream *pflog)
{
    study_assoc_dir *p = new study_assoc_dir(study_uid, path, meta_notify_file, assoc_timeout, pflog);
    studies_map[study_uid] = p;
    return p;
}

study_assoc_dir* study_assoc_dir::find(const string &study_uid)
{
    STUDY_MAP::iterator it = studies_map.find(study_uid);
    if(it != studies_map.end()) return it->second;
    else return NULL;
}

void study_assoc_dir::add_file(np_conn_assoc_dir *p_assoc_dir, const string &hash, const string &unique_filename, const string &p_notify_file, const string &p_instance_file, unsigned int seq)
{
    if(p_assoc_dir && hash.length() && unique_filename.length() && p_notify_file.length() && p_instance_file.length())
    {
        time_header_out(*pflog) << "study_assoc_dir::add_file() append compress_job: " << p_assoc_dir->get_id() << " " << unique_filename << endl;
        compress_queue.push_back(shared_ptr<compress_job>(new compress_job(p_assoc_dir->get_id(), p_assoc_dir->get_path(),
            p_notify_file, hash, unique_filename, p_instance_file, p_assoc_dir->get_expected_syntax(), get_id(), seq, pflog)));
        named_pipe_listener *p = p_assoc_dir->get_listener();
        if(p)
        {
            associations[p_assoc_dir->get_id()] = p->find_connections_read(p_assoc_dir->get_overlap_read());
            refresh_last_access();
        }
    }
    else
    {
        if(p_assoc_dir == NULL) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: p_assoc_dir is NULL." << endl;
        else if(hash.length() == 0) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: hash is NULL." << endl;
        else if(unique_filename.length() == 0) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: unique_filename is NULL." << endl;
        else if(p_notify_file.length() == 0) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: p_notify_file is NULL." << endl;
        else if(p_instance_file.length() == 0) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: p_instance_file is NULL." << endl;
        else time_header_out(*pflog) << "study_assoc_dir::add_file() param error." << endl;
    }
}

void study_assoc_dir::erase(std::list<std::shared_ptr<compress_job> >::const_iterator it)
{
    if(opt_verbose) time_header_out(*pflog) << "study_assoc_dir::erase(" << (*it)->get_unique_filename() << ")" << endl;
    compress_queue.erase(it);
}

pair<study_assoc_dir*, list<shared_ptr<compress_job> >::const_iterator> study_assoc_dir::find_first_job_in_studies(const string &base)
{
    set<string> notify_files;
    study_assoc_dir* ps = NULL;
    list<shared_ptr<compress_job> >::const_iterator ps_pos;
    for(STUDY_MAP::const_iterator it = studies_map.cbegin(); it != studies_map.cend(); ++it)
    {
        if(it->second)
        {
            list<shared_ptr<compress_job> >::const_iterator pos = it->second->get_first_greater_notify_filename(base);
            if(pos != it->second->compress_queue.cend())
            {
                ps = it->second;
                ps_pos = pos;
            }
        }
    }
    return pair<study_assoc_dir*, list<shared_ptr<compress_job> >::const_iterator>(ps, ps_pos);
}
