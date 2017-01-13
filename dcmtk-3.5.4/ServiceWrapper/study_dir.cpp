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

STUDY_POS_PAIR study_dir::find_first_job_in_studies(const string &base)
{
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
