#include "stdafx.h"

using namespace std;
using namespace handle_context;

string study_dir::empty_notify_filename;
STUDY_MAP study_dir::studies_map;

void study_dir::print_state() const
{
    *pflog << "study_dir::print_state() id: " << get_id() << endl
        << "\tassociations:" << endl;
    for(RELATION_MAP::const_iterator it = relations.cbegin(); it != relations.cend(); ++it)
        *pflog << "\t\t" << (it->second == NULL ? "NULL" : it->second->get_id()) << endl;
    *pflog << "\tcompress_queue:" << endl;
    for(FILE_QUEUE::const_iterator it = file_queue.cbegin(); it != file_queue.cend(); ++it)
        *pflog << "\t\t" << it->second->get_path() << "\\" << it->second->get_instance_filename() << "(" << it->second->get_notify_filename() << ") -> " << it->second->get_hash() << "\\*\\" << it->second->get_unique_filename() << endl;
    base_dir::print_state();
}

shared_ptr<study_dir> study_dir::create_instance(const char *study_uid, const char *path, const char *meta_notify_file, ostream *pflog)
{
    shared_ptr<study_dir> p(new study_dir(study_uid, path, meta_notify_file, assoc_timeout, pflog));
    studies_map[study_uid] = p;
    return p;
}

shared_ptr<study_dir> study_dir::find(const string &study_uid)
{
    STUDY_MAP::iterator it = studies_map.find(study_uid);
    if(it != studies_map.end()) return it->second;
    else return NULL;
}

void study_dir::add_file(np_conn_assoc_dir *p_assoc_dir, const string &hash, const string &unique_filename, const string &p_notify_file, const string &p_instance_file, unsigned int seq)
{
    if(p_assoc_dir && hash.length() && unique_filename.length() && p_notify_file.length() && p_instance_file.length())
    {
        time_header_out(*pflog) << "study_dir::add_file() append file_notify: " << p_assoc_dir->get_id() << " " << unique_filename << endl;
        shared_ptr<file_notify> sp_file(new file_notify(p_assoc_dir->get_id(), p_assoc_dir->get_path(),
            p_notify_file, hash, unique_filename, p_instance_file, p_assoc_dir->get_expected_syntax(), get_id(), seq, pflog));
        shared_ptr<np_conn_assoc_dir> pa(p_assoc_dir->get_shared_ptr_assoc_conn());
        if(pa)
        {
            shared_ptr<study_dir> ps(study_dir::find(get_id()));
            if(ps)
            {
                file_queue[sp_file->get_notify_filename()] = sp_file;
                relations[pa->get_id()] = shared_ptr<relationship>(new relationship(pa, ps));
                // todo: refresh relationship's timestamp
                refresh_last_access();
            }
            else time_header_out(*pflog) << "study_dir::add_file() find study " << get_id() << " in studies_map failed." << endl;
        }
        else time_header_out(*pflog) << "study_dir::add_file() assoc " << p_assoc_dir->get_id() << "->get_shared_ptr_assoc_conn() failed." << endl;
    }
    else
    {
        if(p_assoc_dir == NULL) time_header_out(*pflog) << "study_dir::add_file() param error: p_assoc_dir is NULL." << endl;
        else if(hash.length() == 0) time_header_out(*pflog) << "study_dir::add_file() param error: hash is NULL." << endl;
        else if(unique_filename.length() == 0) time_header_out(*pflog) << "study_dir::add_file() param error: unique_filename is NULL." << endl;
        else if(p_notify_file.length() == 0) time_header_out(*pflog) << "study_dir::add_file() param error: p_notify_file is NULL." << endl;
        else if(p_instance_file.length() == 0) time_header_out(*pflog) << "study_dir::add_file() param error: p_instance_file is NULL." << endl;
        else time_header_out(*pflog) << "study_dir::add_file() param error." << endl;
    }
}

void study_dir::erase(FILE_QUEUE::const_iterator it)
{
    if(opt_verbose) time_header_out(*pflog) << "study_dir::erase(" << it->second->get_unique_filename() << ")" << endl;
    file_queue.erase(it);
}

STUDY_POS_PAIR study_dir::find_first_job_in_studies(const string &base)
{
    set<string> notify_files;
    shared_ptr<study_dir> ps;
    FILE_QUEUE::const_iterator ps_pos;
    for(STUDY_MAP::const_iterator it = studies_map.cbegin(); it != studies_map.cend(); ++it)
    {
        if(it->second)
        {
            FILE_QUEUE::const_iterator pos = it->second->get_first_notify_filename_greater(base);
            if(pos != it->second->file_queue.cend() && 
                (ps == NULL || pos->second->get_notify_filename().compare(ps_pos->second->get_notify_filename()) < 0))
            {
                ps = it->second;
                ps_pos = pos;
            }
        }
    }
    return STUDY_POS_PAIR(ps, ps_pos);
}
