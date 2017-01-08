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
    for(list<compress_job>::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
        *pflog << "\t\t" << it->get_path() << "\\" << it->get_instance_filename() << "(" << it->get_notify_filename() << ") -> " << it->get_hash() << "\\*\\" << it->get_unique_filename() << endl;
    base_dir::print_state();
}

study_assoc_dir* study_assoc_dir::create_instance(const char *study_uid, const char *path, const char *meta_notify_file, ostream *pflog)
{
    study_assoc_dir *p = new study_assoc_dir(study_uid, path, meta_notify_file, assoc_timeout, pflog);
    studies_map[study_uid] = p;
    return p;
}

void study_assoc_dir::add_file(np_conn_assoc_dir *p_assoc_dir, const string &hash, const string &unique_filename, const string &p_notify_file, const string &p_instance_file, unsigned int seq)
{
    if(p_assoc_dir && hash.length() && unique_filename.length() && p_notify_file.length() && p_instance_file.length())
    {
        // todo: add instance_file to JOB_TUPLE
        compress_queue.push_back(compress_job(p_assoc_dir->get_id(), p_assoc_dir->get_path(), p_notify_file, hash, unique_filename, p_instance_file, p_assoc_dir->get_expected_syntax(), get_id(), seq));
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

const std::string& study_assoc_dir::get_first_greater_notify_filename(const string &base) const
{
    for(list<compress_job>::const_iterator it = compress_queue.begin(); it != compress_queue.end(); ++it)
    {
        if(it->get_notify_filename().compare(base) > 0) return it->get_notify_filename();
    }
    return empty_notify_filename;
}

const compress_job* study_assoc_dir::get_first_tuple_equal(const std::string &base) const
{
    if(compress_queue.size() == 0) return NULL;
    for(list<compress_job>::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
    {
        int result = it->get_notify_filename().compare(base);
        if(result == 0) return &*it;
        else if(result > 0) return NULL;
        //else result < 0, continue
    }
    return NULL;
}

study_assoc_dir* study_assoc_dir::find_first_job_in_studies(const string &base)
{
    set<string> notify_files;
    study_assoc_dir* ps = NULL;
    for(STUDY_MAP::const_iterator it = studies_map.cbegin(); it != studies_map.cend(); ++it)
    {
        if(it->second)
        {
            const string &str = it->second->get_first_greater_notify_filename(base);
            if(str.length())
            {
                notify_files.insert(str);
                if(str.compare(*notify_files.cbegin()) == 0) ps = it->second;
            }
        }
    }
    return ps;
}
