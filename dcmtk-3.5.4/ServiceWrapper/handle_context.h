#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class file_notify : public base_dir
    {
    private:
        std::string study_uid, assoc_id, hash, unique_filename, expected_xfer, auto_publish;
        long long rec_file_size;
        unsigned int seq;

    public:
        file_notify() : base_dir("", "", "", 0, &std::cerr), rec_file_size(0LL), seq(0) {};
        file_notify(const std::string &study_uid, const std::string &assoc_id, const std::string &path, const std::string &notify_filename, const std::string &xfer, const std::string &auto_publish,
            const std::string &hash, const std::string &unique_filename, const std::string &instance_filename, unsigned int seq, std::ostream *pflog)
            : base_dir(instance_filename, path, notify_filename, 0, pflog), study_uid(study_uid), assoc_id(assoc_id), expected_xfer(xfer), auto_publish(auto_publish),
            hash(hash), unique_filename(unique_filename), seq(seq), rec_file_size(0LL) {};
        file_notify(const file_notify &r) : base_dir(r), study_uid(r.study_uid), assoc_id(r.assoc_id), expected_xfer(r.expected_xfer), auto_publish(r.auto_publish),
            hash(r.hash), unique_filename(r.unique_filename), seq(r.seq), rec_file_size(r.rec_file_size) { };
        file_notify& operator=(const file_notify& r);

        const std::string& get_instance_filename() const { return get_id(); };
        const std::string& get_assoc_id() const { return assoc_id; };
        const std::string& get_hash() const { return hash; };
        const std::string& get_unique_filename() const { return unique_filename; };
        const std::string& get_expected_xfer() const { return expected_xfer; };
        const std::string& get_study_uid() const { return study_uid; };
        unsigned int get_seq() const { return seq; };
        void set_rec_file_size(long long sz) { rec_file_size = sz; };
        void clear();
        virtual ~file_notify();
        virtual void print_state() const;
    };

    typedef std::map<std::string, std::shared_ptr<file_notify> > FILE_QUEUE; // <notify_filename, std::shared_ptr<file_notify> >
    typedef std::pair<std::string, std::shared_ptr<file_notify> > FILE_QUEUE_PAIR;
    class np_conn_assoc_dir;
    class study_dir;

    class relationship : public base_dir
    {
    private:
        std::shared_ptr<np_conn_assoc_dir> sp_assoc;
        std::shared_ptr<study_dir> sp_study;
        bool assoc_disconn;
        FILE_QUEUE file_queue, index_queue;

    public:
        relationship(std::shared_ptr<np_conn_assoc_dir> sp_assoc, std::shared_ptr<study_dir> sp_study, int time_out_diff, std::ostream *pflog);
        virtual ~relationship();
        std::string get_assoc_id() const;
        std::string get_study_uid() const;
        std::shared_ptr<np_conn_assoc_dir> get_sp_assoc() const { return sp_assoc; };
        size_t get_files_count() const { return file_queue.size(); };
        bool add_file_notify(const std::shared_ptr<file_notify> &sp_job);
        std::shared_ptr<file_notify> find_file_notify(const std::string &notify_filename) const;
        FILE_QUEUE::const_iterator get_first_notify_filename_greater(const std::string &base) const;
        FILE_QUEUE::const_iterator get_file_queue_cend() const { return file_queue.cend(); };
        void erase(const std::string &notify_filename);
        void add_file_to_index_queue(const std::shared_ptr<file_notify> &job) { if(job && job->get_notify_filename().length()) index_queue[job->get_notify_filename()] = job; };
        void print_state() const;
        bool is_timeout_or_close();
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
        std::list<std::string> dead_relations;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(const char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps, int timeout) // path is named_pipe path, \\\\.\\pipe\\dcmtk_qr
            : named_pipe_connection(pnps, timeout), pid(0), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        const std::string& get_expected_syntax() const { return expected_syntax; };
        virtual void callback_pipe_closed();
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
        size_t disconnect_timeout_relation(const std::string &study_uid);
        std::shared_ptr<np_conn_assoc_dir> get_shared_ptr_assoc_conn() const;
        std::shared_ptr<relationship> find_relationship_by_study_uid(const std::string &study_uid) const;
    };

    class handle_proc : public base_dir
    {
    private:
        HANDLE hlog;
        std::string exec_cmd, exec_name, log_path;
        PROCESS_INFORMATION procinfo;
		DWORD priority;
        bool starting_process;
    protected:
        void set_exec_cmd(const char *cmd) { exec_cmd = cmd; };
        void close_proc();
    public:
        handle_proc(const std::string &assoc_id, const std::string &cwd, const std::string &notify_file, const std::string &cmd, const std::string &exec_prog_name, std::ostream *plog) 
            : base_dir(assoc_id, cwd, notify_file, 0, plog), hlog(NULL), exec_cmd(cmd), exec_name(exec_prog_name),
            priority(NORMAL_PRIORITY_CLASS), starting_process(false) { memset(&procinfo, 0, sizeof(PROCESS_INFORMATION)); };
        handle_proc(const handle_proc& o) : base_dir(o), hlog(o.hlog), exec_cmd(o.exec_cmd), exec_name(o.exec_name),
			log_path(o.log_path), procinfo(o.procinfo), priority(o.priority), starting_process(false) {};
        
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
        const bool is_starting_process() const { return starting_process; };
        DWORD start_process(bool out_redirect);
    };

    class handle_compress;
    typedef std::list<handle_compress*> PROC_COMPR_LIST;

    class handle_compress : public handle_proc
    {
    private:
        static handle_compress* make_handle_compress(const std::string &study_uid, const std::shared_ptr<relationship> &r, const std::shared_ptr<file_notify> &job, std::ostream &flog);

        std::shared_ptr<relationship> relation;
        std::shared_ptr<file_notify> compr_job;
        
    protected:
        handle_compress(const std::string &id, const std::string &path, const std::string &notify, const std::string &cmd, const std::string &exec_prog_name,
            const std::shared_ptr<relationship> &relation, const std::shared_ptr<file_notify> &job, std::ostream *plog)
            : handle_proc(id, path, notify, cmd, exec_prog_name, plog), relation(relation), compr_job(job) { };

    public:
        static void find_first_file_notify_to_start_process(HANDLE hSema, PROC_COMPR_LIST &proc_list, std::ostream &flog);
        virtual ~handle_compress() { relation->add_file_to_index_queue(compr_job); };
        void print_state() const;
    };

    typedef std::map<std::string, std::shared_ptr<study_dir> > STUDY_MAP;
    typedef std::pair<std::string, std::shared_ptr<study_dir> > STUDY_PAIR;
    typedef std::pair<std::shared_ptr<relationship>, FILE_QUEUE::const_iterator> RELA_POS_PAIR;

    class study_dir : public named_pipe_connection, public handle_proc
    {
    private:
        static STUDY_MAP studies_map;
        static named_pipe_listener *p_listener_study;

        RELATION_MAP relations;

        study_dir(int timeout, const std::string &study_uid, const std::string &hash, const std::string &orders_study_path, const std::string &first_notify_file_in_study);

    public:
        static void set_named_pipe_listener_ptr(named_pipe_listener *p) { p_listener_study = p; };
        static std::shared_ptr<study_dir> create_instance(const std::string &study_uid, const std::string &hash, const std::string &orders_study_path, const std::string &first_notify_file_in_study);
        static std::shared_ptr<study_dir> find(const std::string &study_uid);
        static std::shared_ptr<named_pipe_connection> WINAPI bind_study_by_client_proc_id(named_pipe_listener *pnps, ULONG clientProcId);
        static RELA_POS_PAIR find_first_job_in_studies(const std::string &base);
        static void remove_all_study(std::ostream *pflog);
        static void cleanup(std::ostream *pflog);

        virtual ~study_dir();
        virtual void print_state() const;
        const std::string& get_id() const { return named_pipe_connection::get_id(); };
        const std::string& get_notify_filename() const { return named_pipe_connection::get_notify_filename(); };
        const std::string& get_archdir_path() const { return handle_proc::get_path(); };
        const std::string& get_orders_study_path() const { return handle_proc::get_notify_filename(); }; //relative path
        std::ostream* get_err_stream() const { return named_pipe_connection::get_err_stream(); };
        virtual bool is_time_out() const { return named_pipe_connection::is_time_out(); };

        size_t get_file_queue_count() const { return std::accumulate(relations.cbegin(), relations.cend(), 0,
            [](size_t accu, const RELATION_PAIR &p) { return accu + (p.second ? p.second->get_files_count() : 0); }); };
        void insert_relation(const std::shared_ptr<relationship>& r) { relations[r->get_assoc_id()] = r; };
        void remove_all_relations();
        std::shared_ptr<relationship> find_relationship_by_assoc_id(const std::string &assoc_id) const;
        RELA_POS_PAIR get_first_file_notify_greater(const std::string &base) const;
    };
}

#endif
