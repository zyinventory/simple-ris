#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class study_assoc_dir;
    typedef std::map<std::string, study_assoc_dir*> STUDY_MAP;
    typedef std::pair<std::string, study_assoc_dir*> STUDY_PAIR;
    // <notify_file, notify_path, hash, unique_filename>
    typedef std::tuple<std::string, std::string, std::string, std::string> JOB_TUPLE; 
    typedef std::list<JOB_TUPLE > JOB_LIST;
    class np_conn_assoc_dir;

    class study_assoc_dir : public base_dir
    {
    private:
        static std::string empty_notify_filename;
        static STUDY_MAP studies_map;
        JOB_LIST compress_queue;
        std::set<np_conn_assoc_dir*> associations;

        study_assoc_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : base_dir(study_uid, path, meta_notify_file, timeout, plog) { };

    public:
        static study_assoc_dir* create_instance(const char *study_uid, const char *path, const char *meta_notify_file, std::ostream *pflog);
        static study_assoc_dir* find_first_job_in_studies();
        virtual void print_state() const;
        const std::string& get_first_notify_filename() const { return compress_queue.size() ? std::get<0>(*compress_queue.begin()) : empty_notify_filename; };
        void add_file(np_conn_assoc_dir *p_assoc_dir, const char *hash, const char *unique_filename, const char *p_notify_file);
    };

    class np_conn_study_dir : public named_pipe_connection
    {   // compress proc collector, pick up job from study_assoc_dir
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
        bool disconn_release;
        std::map<std::string, study_assoc_dir*> studies;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps, int timeout) : named_pipe_connection(pnps, timeout), pid(0), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
    };

    class handle_proc : public base_dir
    {
    private:
        HANDLE hlog;
        std::string exec_cmd, exec_name, log_path;
        PROCESS_INFORMATION procinfo;
		DWORD priority;
    public:
        handle_proc(const std::string &assoc_id, const std::string &cwd, const std::string &notify_file, const std::string &cmd, const std::string &exec_prog_name, std::ostream *plog) 
            : base_dir(assoc_id, cwd, notify_file, 0, plog), hlog(NULL), exec_cmd(cmd), exec_name(exec_prog_name), priority(NORMAL_PRIORITY_CLASS)
            { memset(&procinfo, 0, sizeof(PROCESS_INFORMATION)); };
        handle_proc(const handle_proc& o) : base_dir(o), hlog(o.hlog), exec_cmd(o.exec_cmd), exec_name(o.exec_name),
			log_path(o.log_path), procinfo(o.procinfo), priority(o.priority) {};
        
        static bool make_proc_ris_integration(const NOTIFY_FILE_CONTEXT *pnfc, const std::string &prog_path, std::ostream &flog);

        handle_proc& operator=(const handle_proc &r);
		DWORD set_priority(DWORD p);
        void print_state() const;
        virtual ~handle_proc();
        HANDLE get_handle() const { return procinfo.hProcess; };
        const std::string& get_exec_cmd() const { return exec_cmd; };
        const std::string& get_exec_name() const { return exec_name; };
        const std::string& get_log_path() const { return log_path; };
        const PROCESS_INFORMATION& get_procinfo() const { return procinfo; };
        int start_process(bool out_redirect);
    };
}

#endif
