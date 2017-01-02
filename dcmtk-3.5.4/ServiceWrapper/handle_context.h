#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class study_compr_job_dir;
    typedef std::map<std::string, study_compr_job_dir*> STUDY_MAP;
    typedef std::pair<std::string, study_compr_job_dir*> STUDY_PAIR;
    // <notify_file, notify_path>
    typedef std::pair<std::string, std::string> JOB_PAIR; 
    typedef std::list<JOB_PAIR > JOB_LIST;
    class np_conn_assoc_dir;

    class study_compr_job_dir : public base_dir
    {
    private:
        static std::string empty_notify_filename;
        static STUDY_MAP studies_map;
        JOB_LIST compress_queue;
        std::set<np_conn_assoc_dir*> associations;

        study_compr_job_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : base_dir(study_uid, path, meta_notify_file, timeout, plog) { };

    public:
        static study_compr_job_dir* create_instance(const char *study_uid, const char *path, const char *meta_notify_file, std::ostream *pflog);
        static study_compr_job_dir* find_first_job_in_studies();
        virtual void print_state() const;
        const std::string& get_first_notify_filename() const { return compress_queue.size() ? compress_queue.begin()->first : empty_notify_filename; };
        void add_file(np_conn_assoc_dir *p_assoc_dir, const char *p_notify_file);
    };

    class np_conn_study_dir : public named_pipe_connection
    {   // compress proc collector, pick up job from study_compr_job_dir
    private:

    public:
        np_conn_study_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : named_pipe_connection(study_uid, path, meta_notify_file, PIPE_BUFFER_SIZE, PIPE_BUFFER_SIZE, timeout, plog) { };
    };

    class np_conn_assoc_dir : public named_pipe_connection
    {
    private:
        std::string calling, called, remote, port, transfer_syntax, auto_publish;
        DWORD pid;
        bool assoc_disconn, disconn_release;
        std::map<std::string, study_compr_job_dir*> studies;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps, int timeout) : named_pipe_connection(pnps, timeout), pid(0), assoc_disconn(true), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
    };
}

#endif
