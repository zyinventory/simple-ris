#include "stdafx.h"

using namespace std;
using namespace handle_context;

string study_assoc_dir::empty_notify_filename;
STUDY_MAP study_assoc_dir::studies_map;

void study_assoc_dir::print_state() const
{
    *pflog << "study_assoc_dir::print_state() id: " << get_id() << endl
        << "\tassociations:" << endl;
    for(set<np_conn_assoc_dir*>::const_iterator it = associations.cbegin(); it != associations.cend(); ++it)
        *pflog << "\t\t" << (*it == NULL ? "NULL" : (*it)->get_id()) << endl;
    *pflog << "\tcompress_queue:" << endl;
    for(JOB_LIST::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
        *pflog << "\t\t" << get<1>(*it) << "\\" << get<0>(*it) << " -> " << get<2>(*it) << "\\*\\" << get<3>(*it) << endl;
    base_dir::print_state();
}

study_assoc_dir* study_assoc_dir::create_instance(const char *study_uid, const char *path, const char *meta_notify_file, ostream *pflog)
{
    study_assoc_dir *p = new study_assoc_dir(study_uid, path, meta_notify_file, assoc_timeout, pflog);
    studies_map[study_uid] = p;
    return p;
}

void study_assoc_dir::add_file(np_conn_assoc_dir *p_assoc_dir, const char *hash, const char *unique_filename, const char *p_notify_file)
{
    if(p_assoc_dir && hash && unique_filename && p_notify_file)
    {
        compress_queue.push_back(JOB_TUPLE(p_notify_file, p_assoc_dir->get_path(), hash, unique_filename, p_assoc_dir));
        associations.insert(p_assoc_dir);
        refresh_last_access();
    }
    else
    {
        if(p_assoc_dir == NULL) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: p_assoc_dir is NULL." << endl;
        else if(hash == NULL) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: hash is NULL." << endl;
        else if(unique_filename == NULL) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: unique_filename is NULL." << endl;
        else if(p_notify_file == NULL) time_header_out(*pflog) << "study_assoc_dir::add_file() param error: p_notify_file is NULL." << endl;
    }
}

const std::string& study_assoc_dir::get_first_greater_notify_filename(const string &base) const
{
    for(JOB_LIST::const_iterator it = compress_queue.begin(); it != compress_queue.end(); ++it)
    {
        if(get<0>(*it).compare(base) > 0) return get<0>(*it);
    }
    return empty_notify_filename;
}

const JOB_TUPLE* study_assoc_dir::get_first_tuple_equal(const std::string &base) const
{
    if(compress_queue.size() == 0) return NULL;
    for(JOB_LIST::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
    {
        int result = get<0>(*it).compare(base);
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
