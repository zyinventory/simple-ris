#include "stdafx.h"

using namespace std;
using namespace handle_context;

string study_compr_job_dir::empty_notify_filename;
STUDY_MAP study_compr_job_dir::studies_map;

void study_compr_job_dir::print_state() const
{
    *pflog << "study_compr_job_dir::print_state() id: " << get_id() << endl
        << "\tassociations:" << endl;
    for(set<np_conn_assoc_dir*>::const_iterator it = associations.cbegin(); it != associations.cend(); ++it)
        *pflog << "\t\t" << (*it == NULL ? "NULL" : (*it)->get_id()) << endl;
    *pflog << "\tcompress_queue:" << endl;
    for(JOB_LIST::const_iterator it = compress_queue.cbegin(); it != compress_queue.cend(); ++it)
        *pflog << "\t\t" << it->second << "\\" << it->first << endl;
    base_dir::print_state();
}

study_compr_job_dir* study_compr_job_dir::create_instance(const char *study_uid, const char *path, const char *meta_notify_file, ostream *pflog)
{
    study_compr_job_dir *p = new study_compr_job_dir(study_uid, path, meta_notify_file, assoc_timeout, pflog);
    studies_map[study_uid] = p;
    return p;
}

void study_compr_job_dir::add_file(np_conn_assoc_dir *p_assoc_dir, const char *p_notify_file)
{
    if(p_assoc_dir && p_notify_file)
    {
        compress_queue.push_back(JOB_PAIR(p_notify_file, p_assoc_dir->get_path()));
        associations.insert(p_assoc_dir);
        refresh_last_access();
    }
    else
    {
        if(p_assoc_dir == NULL) time_header_out(*pflog) << "study_compr_job_dir::add_file() param error: p_assoc_dir is NULL." << endl;
        else if(p_notify_file == NULL) time_header_out(*pflog) << "study_compr_job_dir::add_file() param error: p_notify_file is NULL." << endl;
    }
}

study_compr_job_dir* study_compr_job_dir::find_first_job_in_studies()
{
    set<string> notify_files;
    transform(studies_map.cbegin(), studies_map.cend(), inserter(notify_files, notify_files.end()), [](const STUDY_PAIR &p) -> const string& {
        if(p.second == NULL) return empty_notify_filename;
        return p.second->get_first_notify_filename();
    });
    if(notify_files.size() == 0) return NULL;
    set<string>::const_iterator itnf = notify_files.cbegin();
    if(itnf->length() == 0) ++itnf;
    const string& first = *itnf;
    STUDY_MAP::const_iterator it = find_if(studies_map.cbegin(), studies_map.cend(), [&first](const STUDY_PAIR &p) -> bool {
        if(p.second == NULL) return false;
        return (first.compare(p.second->get_first_notify_filename()) == 0);
    });
    if(it == studies_map.cend()) return NULL;
    else return it->second;
}
