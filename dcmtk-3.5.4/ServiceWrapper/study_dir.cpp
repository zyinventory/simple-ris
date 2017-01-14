#include "stdafx.h"

using namespace std;
using namespace handle_context;

STUDY_MAP study_dir::studies_map;

void study_dir::print_state() const
{
    *pflog << "study_dir::print_state() id: " << get_id() << endl
        << "\trelations:" << endl;
    for_each(relations.cbegin(), relations.cend(), [](const RELATION_PAIR &p) { if(p.second) p.second->print_state(); });
    base_dir::print_state();
}

shared_ptr<relationship> study_dir::find_relationship_by_assoc_id(const string &assoc_id) const
{
    RELATION_MAP::const_iterator it = relations.find(assoc_id);
    if(it != relations.cend()) return it->second;
    else return NULL;
}

void study_dir::remove_all_relations()
{
    RELATION_MAP::const_iterator it = relations.cbegin();
    if(it != relations.cend())
    {
        if(opt_verbose) time_header_out(*pflog) << "study_dir::remove_all_relations(" << get_id() << ")..." << endl;
        while(it != relations.cend()) it = relations.erase(it);
        if(opt_verbose) time_header_out(*pflog) << "study_dir::remove_all_relations() OK." << endl;
    }
}

study_dir::~study_dir()
{
    time_header_out(*pflog) << "study_dir::~study_dir(" << get_id() << ")..." << endl;
    remove_all_relations();
}

shared_ptr<study_dir> study_dir::create_instance(const std::string &study_uid, const std::string &path, const std::string &meta_notify_file, ostream *pflog)
{
    shared_ptr<study_dir> p(new study_dir(study_uid, path, meta_notify_file, store_timeout, pflog));
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

void study_dir::cleanup(std::ostream *pflog)
{
    // begin disconnect all timeout assoc
    STUDY_MAP::const_iterator its = studies_map.cbegin();
    while(its != studies_map.cend())
    {
        if(its->second == NULL) { its = studies_map.erase(its); continue; }
        shared_ptr<study_dir> ps(its->second);
        
        bool all_relations_disconn = true;
        RELATION_MAP::iterator itr = ps->relations.begin();
        while(itr != ps->relations.end())
        {
            if(itr->second == NULL) { itr = ps->relations.erase(itr); continue; }
            shared_ptr<relationship> pr(itr->second);
            std::shared_ptr<np_conn_assoc_dir> sp_assoc = pr->get_sp_assoc();
            if(sp_assoc == NULL) { itr = ps->relations.erase(itr); continue; }

            bool is_timeout = pr->is_timeout_or_close();
            if(is_timeout) sp_assoc->disconnect_timeout_relation(its->first);
            all_relations_disconn &= is_timeout;
            ++itr; //next relation
        }
        if(all_relations_disconn)
        {
            if(ps->is_time_out() && ps->get_file_queue_count() == 0)
            {
                //todo: clear index_queue before ps->remove_all_relations
                ps->remove_all_relations();
            }
        }
        ++its; //next study
    }
    // end disconnect all timeout assoc
}
