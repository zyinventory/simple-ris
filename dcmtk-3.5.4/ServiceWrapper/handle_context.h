#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class study_assoc_dir;
    typedef std::map<std::string, study_assoc_dir*> STUDY_MAP;
    typedef std::pair<std::string, study_assoc_dir*> STUDY_PAIR;
        
    class compress_job
    {
    private:
        std::string assoc_id, path, notify_filename, hash, unique_filename, instance_filename;

    public:
        compress_job(const std::string &assoc_id, const std::string &path, const std::string &notify_filename,
            const std::string &hash, const std::string &unique_filename, const std::string &instance_filename)
            : unique_filename(unique_filename), path(path), notify_filename(notify_filename), hash(hash), assoc_id(assoc_id) {};
        compress_job(const compress_job &r) { *this = r; };
        compress_job& operator=(const compress_job& r)
        {
            assoc_id = r.assoc_id;
            path = r.path;
            notify_filename = r.notify_filename;
            hash = r.hash;
            unique_filename = r.unique_filename;
            instance_filename = r.instance_filename;
            return *this;
        };
        const std::string& get_assoc_id() const { return assoc_id; };
        const std::string& get_path() const { return path; };
        const std::string& get_notify_filename() const { return notify_filename; };
        const std::string& get_hash() const { return hash; };
        const std::string& get_unique_filename() const { return unique_filename; };
        const std::string& get_instance_filename() const { return instance_filename; };
    };

    class np_conn_assoc_dir;

    class study_assoc_dir : public base_dir
    {
    private:
        static std::string empty_notify_filename;
        static STUDY_MAP studies_map;
        std::list<compress_job> compress_queue;
        std::set<np_conn_assoc_dir*> associations;

        study_assoc_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : base_dir(study_uid, path, meta_notify_file, timeout, plog) { };

    public:
        static study_assoc_dir* create_instance(const char *study_uid, const char *path, const char *meta_notify_file, std::ostream *pflog);
        static study_assoc_dir* find_first_job_in_studies(const std::string &base);
        virtual void print_state() const;
        void add_file(np_conn_assoc_dir *p_assoc_dir, const char *hash, const char *unique_filename, const char *p_notify_file, const char *p_instance_file);
        const std::string& get_first_greater_notify_filename(const std::string &base) const;
        bool find_notify_filename(const std::string &base) const { return get_first_tuple_equal(base) != NULL; };
        const compress_job* get_first_tuple() const { return compress_queue.size() ? &compress_queue.front() : NULL; };
        const compress_job* get_first_tuple_equal(const std::string &base) const;
        void pop_front_tuple() { compress_queue.pop_front(); };
    };

    class np_conn_assoc_dir : public named_pipe_connection
    {
    private:
        std::string calling, called, remote, transfer_syntax, auto_publish;
        int port;
        DWORD pid;
        bool disconn_release;
        std::map<std::string, study_assoc_dir*> studies;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(const char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps, int timeout) : named_pipe_connection(pnps, timeout), pid(0), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
        void fill_association(NOTIFY_FILE_CONTEXT *pnfc) const;
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
    typedef std::list<handle_proc*> HANDLE_PROC_LIST;

    class handle_compress : public handle_proc
    {
    private:
        NOTIFY_FILE_CONTEXT *notify_ctx;
        
    protected:
        handle_compress(const std::string &id, const std::string &path, const std::string &notify, const std::string &cmd, const std::string &exec_prog_name, NOTIFY_FILE_CONTEXT *pnfc, std::ostream *plog)
            : handle_proc(id, path, notify, cmd, exec_prog_name, plog), notify_ctx(pnfc) { };

    public:
        static handle_compress* make_handle_compress(NOTIFY_FILE_CONTEXT *pnfc, std::ostream &flog);
        virtual ~handle_compress() { if(notify_ctx) delete notify_ctx; };
        void print_state() const;
        NOTIFY_FILE_CONTEXT* get_notify_context() { return notify_ctx; };
    };

}

#endif
