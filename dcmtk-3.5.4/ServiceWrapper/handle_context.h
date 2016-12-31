#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

#include <time.h>
#include <string>

namespace handle_context
{
    typedef std::pair<HANDLE, base_dir*> BASE_HANDLE_PAIR;
    class handle_compress;
    typedef std::map<HANDLE, handle_compress*> HANDLE_MAP;
    typedef std::pair<HANDLE, handle_compress*> HANDLE_PAIR;
    class handle_dir;
    typedef std::map<std::string, handle_dir*> HANDLE_DIR_MAP;
    typedef std::pair<std::string, handle_dir*> HANDLE_DIR_PAIR;
    class handle_proc;
    typedef std::list<handle_proc*> HANDLE_PROC_LIST;
    class handle_study;
    typedef std::map<std::string, handle_study*> STUDY_MAP;
    typedef std::pair<std::string, handle_study*> STUDY_PAIR;

    enum ACTION_TYPE { NO_ACTION = 0, BURN_PER_STUDY, BURN_MULTI, INDEX_INSTANCE, NO_INSTANCE };

    class action_from_association : public base_path
    {
        friend class std::hash<action_from_association>;
    public:
        ACTION_TYPE type;
        bool release;
        std::string burn_multi_id;
        NOTIFY_FILE_CONTEXT *pnfc;

        static const char *translate_action_type(ACTION_TYPE t);

        action_from_association(ACTION_TYPE action_type, std::string path, bool is_release, std::ostream *plog)
            : base_path(path, plog), type(action_type), pnfc(NULL), release(is_release) {};
        action_from_association(const NOTIFY_FILE_CONTEXT &nfc, std::string path, std::ostream *plog)
            : base_path(path, plog), type(INDEX_INSTANCE), release(false) { pnfc = new NOTIFY_FILE_CONTEXT; *pnfc = nfc; };
        action_from_association(const action_from_association& r)
            : base_path(r), type(r.type), burn_multi_id(r.burn_multi_id), release(r.release)
        {
            if(r.pnfc) { pnfc = new NOTIFY_FILE_CONTEXT; *pnfc = *r.pnfc; }
            else pnfc = NULL;
        };
        virtual ~action_from_association() { if(pnfc) delete pnfc; };
        action_from_association& operator=(const action_from_association &r);
        void print_state() const;
        bool operator<(const action_from_association &r) const;
        bool operator==(const action_from_association &r) const { return (!(*this < r) && !(r < *this)); };
        ACTION_TYPE get_type() const { return type; };
        const NOTIFY_FILE_CONTEXT* get_notify_file_context() const { return pnfc; };
        bool is_disconnect() const { return type == NO_ACTION || type == BURN_PER_STUDY; };
        bool is_auto_publish() const { return release && type == BURN_PER_STUDY; };
    };
    
    class named_pipe_server;

    class handle_dir : public base_dir // handle_dir is association
    {
    private:
        std::list<std::string> list_file;
        std::set<std::string> set_complete, set_study; // set_study: association[1] -> study[n]
        bool last_find_error, assoc_disconn, disconn_release;
        std::string last_association_notify_filename;
        NOTIFY_ASSOC_SECTION assoc;
        
        void process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog);
        NOTIFY_FILE_CONTEXT* process_notify_file(std::istream &ifs, unsigned int file_tag, std::ostream &flog);
        void fill_association_section(NOTIFY_ASSOC_SECTION &nas) const { nas = assoc; };

    public:
        handle_dir(const std::string &assoc_id, const std::string &path, const std::string notify_filename, std::ostream *plog)
            : base_dir(assoc_id, path, notify_filename, plog), last_find_error(false), last_association_notify_filename(notify_filename),
            assoc_disconn(false), disconn_release(true) { memset(&assoc, 0, sizeof(NOTIFY_ASSOC_SECTION)); assoc.port = 104; };
        handle_dir(const handle_dir& o) : base_dir(o), last_find_error(o.last_find_error),
            list_file(o.list_file), set_complete(o.set_complete), set_study(o.set_study), last_association_notify_filename(o.last_association_notify_filename),
            assoc_disconn(o.assoc_disconn), disconn_release(o.disconn_release), assoc(o.assoc) {};
        virtual ~handle_dir();

        bool is_time_out() const;
        handle_dir& operator=(const handle_dir &r);
        void print_state() const;
        bool insert_study(const std::string &study_uid) { return set_study.insert(study_uid).second; };
        void remove_file_from_list(const std::string &filename) { list_file.remove(filename); };
        std::string& get_find_filter(std::string&) const;
        bool is_last_find_error() const { return last_find_error; };
        bool is_association_disconnect() const { return assoc_disconn; };
        //bool is_disconnect_release() const { return disconn_release; };
        int file_complete_remain() const { return list_file.size() - set_complete.size(); };
        DWORD find_files(std::ostream &flog, std::function<DWORD(const std::string&)> p);
        DWORD process_notify(const std::string &filename, NOTIFY_LIST &compress_queue, std::ostream &flog);
        void send_compress_complete_notify(const NOTIFY_FILE_CONTEXT &nfc, bool compress_ok, std::ostream &flog);
        bool is_normal_close() const { return assoc_disconn && disconn_release; };
        void broadcast_assoc_close_action_to_all_study(named_pipe_server &nps) const;
        void send_all_compress_ok_notify();
        const char* handle_dir::close_description() const;
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
            : base_dir(assoc_id, cwd, notify_file, plog), hlog(NULL), exec_cmd(cmd), exec_name(exec_prog_name), priority(NORMAL_PRIORITY_CLASS)
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
        NOTIFY_FILE_CONTEXT notify_ctx;
        
    protected:
        handle_compress(const std::string &assoc_id, const std::string &path, const std::string &notify_file, const std::string &cmd, const std::string &exec_prog_name, const NOTIFY_FILE_CONTEXT &nfc, std::ostream *plog)
            : handle_proc(assoc_id, path, notify_file, cmd, exec_prog_name, plog), notify_ctx(nfc) { };

    public:
        static handle_compress* make_handle_compress(const NOTIFY_FILE_CONTEXT &nfc, std::ostream &flog);
        handle_compress(const handle_compress& o) : handle_proc(o), notify_ctx(o.notify_ctx) {};
        handle_compress& operator=(const handle_compress &r);
        void print_state() const;
        NOTIFY_FILE_CONTEXT& get_notify_context() { return notify_ctx; };
    };

    typedef struct _tag_PIPEINST
    {
	    OVERLAPPED oOverlap;
	    HANDLE hPipeInst;
	    TCHAR chBuffer[FILE_BUF_SIZE];
	    DWORD cbShouldWrite;
        char study_uid[65];
    } PIPEINST, *LPPIPEINST;

    class handle_study : public handle_proc
    {
    private:
        LPPIPEINST pipe_context;
        bool blocked, ris_integration_start;
        std::string lock_file_name;
        std::list<action_from_association> list_action;
        std::set<std::string> set_association_path;
        action_from_association last_association_action;
        bool open_study_lock_file(std::map<std::string, std::string> &map_ini);
        void save_and_close_lock_file(std::map<std::string, std::string> &map_ini);

    public:
        handle_study(const std::string &cwd, const std::string &cmd, const std::string &exec_prog_name, const std::string &study, std::ostream *plog);
        handle_study(const handle_study &r) : handle_proc(r), pipe_context(r.pipe_context), lock_file_name(r.lock_file_name),
            set_association_path(r.set_association_path), list_action(r.list_action), blocked(r.blocked),
            ris_integration_start(r.ris_integration_start), last_association_action(r.last_association_action) {};
        
        virtual ~handle_study();
        handle_study& operator=(const handle_study &r);
        void print_state() const;
        void bind_pipe_context(LPPIPEINST p_context) { pipe_context = p_context; };
        DWORD write_message_to_pipe();
        void remove_compress_ok_action(const std::string &filename, const std::string &xfer);
        const action_from_association& get_last_association_action() const { return last_association_action; };
        const std::set<std::string>& get_set_association_path() const { return set_association_path; };
        bool insert_association_path(const std::string &assoc_path);
        void remove_association_path(const std::string &assoc_path);
        //DWORD instance_add_to_dicomdir_ok(const std::string &filename, const std::string &xfer_new, LPOVERLAPPED_COMPLETION_ROUTINE write_complete_callback, std::ostream &flog);
        DWORD append_action(const action_from_association &action);
        bool is_time_out() const;
        bool send_remain_message_to_pipe();
    };

    class named_pipe_server : public base_path
    {
    private:
        static named_pipe_server *singleton_ptr;
        HANDLE hPipeEvent, hPipe; // hPipe will change per listening
        OVERLAPPED olPipeConnectOnly;
        STUDY_MAP map_study;

    public:
        named_pipe_server(const char *pipe_path, std::ostream *plog);
        named_pipe_server(const named_pipe_server &r) : base_path(r),
            hPipeEvent(r.hPipeEvent), hPipe(r.hPipe), olPipeConnectOnly(r.olPipeConnectOnly) {};
        named_pipe_server& operator=(const named_pipe_server &r);
        virtual ~named_pipe_server();

        static void register_named_pipe_server(named_pipe_server *p);
        static named_pipe_server* get_named_pipe_server_singleton() { return singleton_ptr; };
        static void CALLBACK client_connect_callback_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        static void CALLBACK read_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        static void CALLBACK write_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);

        bool check_reading_message(LPPIPEINST lpPipeInst, DWORD cbBytesRead, std::string &studyUID, std::string &filename, std::string &xfer, bool confirm_study_uid = true);
        HANDLE get_handle() const { return hPipeEvent; };
        DWORD start_listening();
        DWORD pipe_client_connect_incoming();
        void client_connect_callback(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        void read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        void write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);
        //void disconnect_connection(const std::string &study_uid);
        void disconnect_connection_auto_detect(LPPIPEINST lpPipeInst);
        handle_study* make_handle_study(const std::string &study);
        handle_study* find_handle_study(const std::string &study) { return map_study[study]; };
        void check_study_timeout_to_generate_jdf(const std::set<std::string> &queued_study_uids, const std::set<std::string> &exist_assoc_paths);
    };
}

template<> class std::hash<handle_context::action_from_association>
{
public:
    size_t operator()(const handle_context::action_from_association &c) const
    {
        if(c.pnfc)
        {
            string src_notify(c.pnfc->src_notify_filename),
                unique_filename(c.pnfc->file.unique_filename),
                filename(c.pnfc->file.filename);
            return c.type + hash<string>()(c.burn_multi_id) + hash<string>()(src_notify) 
                + hash<string>()(unique_filename) + hash<string>()(filename);
        }
        else return c.type + hash<string>()(c.burn_multi_id);
    }
};

#endif
