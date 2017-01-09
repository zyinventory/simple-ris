#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class compress_job
    {
    private:
        std::string assoc_id, path, notify_filename, hash, unique_filename, instance_filename, expected_xfer, study_uid;
        long long rec_file_size;
        unsigned int seq;
    public:
        compress_job() : rec_file_size(0LL), seq(0) {};
        compress_job(const std::string &assoc_id, const std::string &path, const std::string &notify_filename, const std::string &hash,
            const std::string &unique_filename, const std::string &instance_filename, const std::string &xfer, const std::string &study_uid, unsigned int seq)
            : unique_filename(unique_filename), path(path), notify_filename(notify_filename), hash(hash), study_uid(study_uid),
            assoc_id(assoc_id), instance_filename(instance_filename), expected_xfer(xfer), rec_file_size(0LL), seq(seq) {};
        compress_job(const compress_job &r) { *this = r; };
        compress_job& operator=(const compress_job& r)
        {
            assoc_id = r.assoc_id;
            path = r.path;
            notify_filename = r.notify_filename;
            hash = r.hash;
            unique_filename = r.unique_filename;
            instance_filename = r.instance_filename;
            expected_xfer = r.expected_xfer;
            rec_file_size = r.rec_file_size;
            study_uid = r.study_uid;
            seq = r.seq;
            return *this;
        };
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
        void clear()
        {
            assoc_id.clear(); path.clear(); notify_filename.clear(); hash.clear();
            unique_filename.clear(); instance_filename.clear(); expected_xfer.clear();
            rec_file_size = 0LL;
        };
    };

    class np_conn_assoc_dir;
    class study_assoc_dir;
    typedef std::map<std::string, std::shared_ptr<named_pipe_connection> > STR_SHARED_CONN_MAP;
    typedef std::pair<std::string, std::shared_ptr<named_pipe_connection> > STR_SHARED_CONN_PAIR;
    typedef std::map<std::string, study_assoc_dir*> STUDY_MAP;
    typedef std::pair<std::string, study_assoc_dir*> STUDY_PAIR;

    class study_assoc_dir : public base_dir
    {
    private:
        static std::string empty_notify_filename;
        static STUDY_MAP studies_map;
        std::list<std::shared_ptr<compress_job> > compress_queue;
        STR_SHARED_CONN_MAP associations;

        study_assoc_dir(const char *study_uid, const char *path, const char *meta_notify_file, int timeout, std::ostream *plog)
            : base_dir(study_uid, path, meta_notify_file, timeout, plog) { };
        std::list<std::shared_ptr<compress_job> >::const_iterator get_first_greater_notify_filename(const std::string &base) const {
            return std::find_if(compress_queue.cbegin(), compress_queue.cend(), [&base](const std::shared_ptr<compress_job> &job) {
                return job->get_notify_filename().compare(base) > 0; }); };

    public:
        static study_assoc_dir* create_instance(const char *study_uid, const char *path, const char *meta_notify_file, std::ostream *pflog);
        static std::pair<study_assoc_dir*, std::list<std::shared_ptr<compress_job> >::const_iterator> find_first_job_in_studies(const std::string &base);
        virtual void print_state() const;
        std::list<std::shared_ptr<compress_job> >::const_iterator get_compress_queue_cend() const { return compress_queue.cend(); };
        void add_file(np_conn_assoc_dir *p_assoc_dir, const std::string &hash, const std::string &unique_filename, const std::string &p_notify_file, const std::string &p_instance_file, unsigned int seq);
        void erase(std::list<std::shared_ptr<compress_job> >::const_iterator it) { compress_queue.erase(it); };
    };

    class np_conn_assoc_dir : public named_pipe_connection
    {
    private:
        std::string calling, called, remote, expected_syntax, auto_publish;
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
        const std::string& get_expected_syntax() const { return expected_syntax; };
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

    class handle_compress : public handle_proc
    {
    private:
        std::shared_ptr<compress_job> compr_job;
        
    protected:
        handle_compress(const std::string &id, const std::string &path, const std::string &notify, const std::string &cmd,
            const std::string &exec_prog_name, const std::shared_ptr<compress_job> &job, std::ostream *plog)
            : handle_proc(id, path, notify, cmd, exec_prog_name, plog), compr_job(job) { };

    public:
        static handle_compress* make_handle_compress(const std::string &study_uid, const std::shared_ptr<compress_job> &job, std::ostream &flog);
        virtual ~handle_compress() {  };
        void print_state() const;
    };
    typedef std::list<handle_compress*> HANDLE_PROC_LIST;
}

#endif
