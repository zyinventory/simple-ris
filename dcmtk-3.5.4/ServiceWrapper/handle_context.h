#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class file_notify
    {
    private:
        std::string study_uid, assoc_id, path, notify_filename, hash, unique_filename, instance_filename, expected_xfer, auto_publish;
        long long rec_file_size;
        unsigned int seq;
        std::ostream *pflog;

    public:
        file_notify() : rec_file_size(0LL), seq(0), pflog(&std::cerr) {};
        file_notify(const std::string &study_uid, const std::string &assoc_id, const std::string &path, const std::string &notify_filename, const std::string &xfer, const std::string &auto_publish,
            const std::string &hash, const std::string &unique_filename, const std::string &instance_filename, unsigned int seq, std::ostream *pflog)
            : study_uid(study_uid), assoc_id(assoc_id), path(path), notify_filename(notify_filename), expected_xfer(xfer), auto_publish(auto_publish),
            instance_filename(instance_filename), hash(hash), unique_filename(unique_filename), seq(seq),
            rec_file_size(0LL), pflog(pflog) {};
        file_notify(const file_notify &r) { *this = r; };
        file_notify& operator=(const file_notify& r);
        const std::string& get_assoc_id() const { return assoc_id; };
        const std::string& get_path() const { return path; };
        const std::string& get_notify_filename() const { return notify_filename; };
        const std::string& get_hash() const { return hash; };
        const std::string& get_unique_filename() const { return unique_filename; };
        const std::string& get_instance_filename() const { return instance_filename; };
        const std::string& get_expected_xfer() const { return expected_xfer; };
        const std::string& get_study_uid() const { return study_uid; };
        unsigned int get_seq() const { return seq; };
        void set_rec_file_size(long long sz) { rec_file_size = sz; };
        void clear();
        virtual ~file_notify();
    };

    typedef std::map<std::string, std::shared_ptr<file_notify> > FILE_QUEUE; // <notify_filename, std::shared_ptr<file_notify> >
    typedef std::pair<std::string, std::shared_ptr<file_notify> > FILE_QUEUE_PAIR;
    class np_conn_assoc_dir;
    class study_dir;

    class relationship
    {
    private:
        std::shared_ptr<np_conn_assoc_dir> sp_assoc;
        std::shared_ptr<study_dir> sp_study;
        FILE_QUEUE file_queue;
        std::ostream *pflog;

    public:
        relationship(std::shared_ptr<np_conn_assoc_dir> sp_assoc, std::shared_ptr<study_dir> sp_study, std::ostream *pflog)
            : sp_assoc(sp_assoc), sp_study(sp_study), pflog(pflog) { };
        std::string get_assoc_id() const;
        std::string get_study_uid() const;
        std::string get_id() const;
        bool add_file_notify(const std::shared_ptr<file_notify> &sp_job);
        std::shared_ptr<file_notify> find_file_notify(const std::string &notify_filename) const;
        FILE_QUEUE::const_iterator get_first_notify_filename_greater(const std::string &base) const;
        FILE_QUEUE::const_iterator get_file_queue_cend() const { return file_queue.cend(); };
        void erase(FILE_QUEUE::const_iterator it);
    };

    typedef std::map<std::string, std::shared_ptr<relationship> > RELATION_MAP;
    typedef std::pair<std::string, std::shared_ptr<relationship> > RELATION_PAIR;

    class np_conn_assoc_dir : public named_pipe_connection
    {
    private:
        std::string calling, called, remote, expected_syntax, auto_publish;
        int port;
        DWORD pid;
        bool disconn_release;
        RELATION_MAP relations;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(const char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps, int timeout) : named_pipe_connection(pnps, timeout), pid(0), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        const std::string& get_expected_syntax() const { return expected_syntax; };
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
        std::shared_ptr<np_conn_assoc_dir> get_shared_ptr_assoc_conn() const
        {
            std::shared_ptr<named_pipe_connection> sp = get_shared_ptr_server_conn();
            if(sp) return std::shared_ptr<np_conn_assoc_dir>(sp, dynamic_cast<np_conn_assoc_dir*>(sp.get()));
            else return NULL;
        };
        std::shared_ptr<relationship> find_relationship_by_study_uid(const std::string &study_uid) const
        {
            RELATION_MAP::const_iterator it = relations.find(study_uid);
            if(it != relations.cend()) return it->second;
            else return NULL;
        };
    };

    typedef std::map<std::string, std::shared_ptr<study_dir> > STUDY_MAP;
    typedef std::pair<std::shared_ptr<study_dir>, FILE_QUEUE::const_iterator> STUDY_POS_PAIR;

    class study_dir : public base_dir
    {
    private:
        static std::string empty_notify_filename;
        static STUDY_MAP studies_map;

        FILE_QUEUE file_queue;
        RELATION_MAP relations;

        study_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : base_dir(study_uid, path, meta_notify_file, timeout, plog) { };

    public:
        static std::shared_ptr<study_dir> create_instance(const char *study_uid, const char *path, const char *meta_notify_file, std::ostream *pflog);
        static std::shared_ptr<study_dir> find(const std::string &study_uid);
        static STUDY_POS_PAIR find_first_job_in_studies(const std::string &base);
        virtual void print_state() const;
        void insert_relation(const std::shared_ptr<relationship>& r) { relations[r->get_assoc_id()] = r; };
        std::shared_ptr<relationship> find_relationship_by_assoc_id(const std::string &assoc_id) const
        {
            RELATION_MAP::const_iterator it = relations.find(assoc_id);
            if(it != relations.cend()) return it->second;
            else return NULL;
        };

        // dummy method
        FILE_QUEUE::const_iterator get_first_notify_filename_greater(const std::string &base) const { return file_queue.cend(); };
        FILE_QUEUE::const_iterator get_file_queue_cend() const { return file_queue.cend(); };
        void erase(FILE_QUEUE::const_iterator it) { };
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

    class handle_compress : public handle_proc
    {
    private:
        std::shared_ptr<file_notify> compr_job;
        
    protected:
        handle_compress(const std::string &id, const std::string &path, const std::string &notify, const std::string &cmd,
            const std::string &exec_prog_name, const std::shared_ptr<file_notify> &job, std::ostream *plog)
            : handle_proc(id, path, notify, cmd, exec_prog_name, plog), compr_job(job) { };

    public:
        static handle_compress* make_handle_compress(const std::string &study_uid, const std::shared_ptr<file_notify> &job, std::ostream &flog);
        virtual ~handle_compress() {  };
        void print_state() const;
    };
    typedef std::list<handle_compress*> HANDLE_PROC_LIST;
}

#endif
