#include "stdafx.h"

using namespace std;
using namespace handle_context;

string study_dir::empty_notify_filename;
STUDY_MAP study_dir::studies_map;

void study_dir::print_state() const
{
    *pflog << "study_dir::print_state() id: " << get_id() << endl
        << "\trelations:" << endl;
    for_each(relations.cbegin(), relations.cend(), [](const RELATION_PAIR &p) { if(p.second) p.second->print_state(); });
    base_dir::print_state();
}

void study_dir::remove_all_relations()
{
    time_header_out(*pflog) << "study_dir::remove_all_relations(" << get_id() << ")..." << endl;
    RELATION_MAP::const_iterator it = relations.cbegin();
    while(it != relations.cend()) it = relations.erase(it);
}

study_dir::~study_dir()
{
    time_header_out(*pflog) << "study_dir::~study_dir(" << get_id() << ")..." << endl;
    remove_all_relations();
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

RELA_POS_PAIR study_dir::get_first_file_notify_greater(const std::string &base) const
{
    shared_ptr<relationship> sp_r;
    FILE_QUEUE::const_iterator it_pos;
    for(RELATION_MAP::const_iterator it_r = relations.cbegin(); it_r != relations.cend(); ++it_r)
    {
        if(it_r->second == NULL) continue;
        FILE_QUEUE::const_iterator pos = it_r->second->get_first_notify_filename_greater(base);
        if(pos != it_r->second->get_file_queue_cend() && (sp_r == NULL || pos->first.compare(it_pos->first) < 0))
        {
            sp_r = it_r->second;
            it_pos = pos;
        }
    }
    return RELA_POS_PAIR(sp_r, it_pos);
}

RELA_POS_PAIR study_dir::find_first_job_in_studies(const string &base)
{
    RELA_POS_PAIR p_pos;
    for(STUDY_MAP::const_iterator it = studies_map.cbegin(); it != studies_map.cend(); ++it)
    {
        if(it->second)
        {
            RELA_POS_PAIR p_new(it->second->get_first_file_notify_greater(base));
            if(p_new.first && (p_pos.first == NULL || p_new.second->first.compare(p_pos.second->first) < 0))
            {
                p_pos = p_new;
            }
        }
    }
    return p_pos;
}

void study_dir::remove_all_study(ostream *pflog)
{
    time_header_out(*pflog) << "study_dir::remove_all_study()..." << endl;
    STUDY_MAP::const_iterator it = studies_map.cbegin();
    while(it != studies_map.cend())
    {
        if(it->second) it->second->remove_all_relations();
        it = studies_map.erase(it);
    }
}
