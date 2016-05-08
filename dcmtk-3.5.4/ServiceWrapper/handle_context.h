#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

#include <time.h>
#include <string>

#define NOTIFY_BASE "store_notify"

namespace MSXML2
{
    struct IXMLDOMDocument2;
}

namespace handle_context
{
    typedef struct _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION {
        char filename[MAX_PATH], hash[12], unique_filename[65], patientID[65], studyUID[65], seriesUID[65], instanceUID[65], xfer[16], xfer_new[16];
        bool isEncapsulated;
        const char* StorePath(char sp = '\\');
        char PathSeparator() const { return unique_filename[8]; }
    } NOTIFY_FILE_CONTEXT_FILE_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_PATIENT_SECTION {
        char patientID[65], patientsName[65], birthday[9], height[10], weight[10], sex[3];
    } NOTIFY_FILE_CONTEXT_PATIENT_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_STUDY_SECTION {
        char studyUID[65], studyDate[9], studyTime[15], accessionNumber[65];
    } NOTIFY_FILE_CONTEXT_STUDY_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_SERIES_SECTION {
        char seriesUID[65], modality[17];
    } NOTIFY_FILE_CONTEXT_SERIES_SECTION;

    typedef struct _tag_NOTIFY_ASSOC_SECTION {
        char id[32], callingAE[65], callingAddr[65], calledAE[65], calledAddr[65], path[MAX_PATH];
        unsigned short port;
    } NOTIFY_ASSOC_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT {
        NOTIFY_ASSOC_SECTION assoc; // FK to association
        char src_notify_filename[64];
        unsigned int file_seq;
        NOTIFY_FILE_CONTEXT_FILE_SECTION file;
        NOTIFY_FILE_CONTEXT_PATIENT_SECTION patient;
        NOTIFY_FILE_CONTEXT_STUDY_SECTION study;
        NOTIFY_FILE_CONTEXT_SERIES_SECTION series;
        bool operator<(const struct _tag_NOTIFY_FILE_CONTEXT &r) const;
        bool operator==(const struct _tag_NOTIFY_FILE_CONTEXT &r) const { return (!(*this < r) && !(r < *this)); };
    } NOTIFY_FILE_CONTEXT;

    typedef std::list<NOTIFY_FILE_CONTEXT> NOTIFY_LIST;

    class base_path
    {
    private:
        std::string path;

    protected:
        std::ostream *pflog;
        base_path(const std::string p, std::ostream *plog) : path(p), pflog(plog) { if(pflog == NULL) pflog = &std::cerr; };

    public:
        base_path(const base_path &r) : path(r.path), pflog(r.pflog) {};
        base_path& operator=(const base_path &r) { path = r.path; pflog = r.pflog; return *this; };
        virtual void print_state() const;
        virtual HANDLE get_handle() const { return NULL; };
        const std::string& get_path() const { return path; };
    };

    class meta_notify_file : public base_path
    {
    private:
        std::string association_id, meta_notify_filename;

    protected:
        meta_notify_file(const std::string &assoc_id, const std::string &p, std::ostream *plog)
            : base_path(p, plog), association_id(assoc_id) {};
        meta_notify_file(const std::string &assoc_id, const std::string &p, const std::string &filename, std::ostream *plog)
            : base_path(p, plog), association_id(assoc_id), meta_notify_filename(filename) {};

    public:
        meta_notify_file(const meta_notify_file& r) : base_path(r), association_id(r.association_id), meta_notify_filename(r.meta_notify_filename) {};
        meta_notify_file& operator=(const meta_notify_file &r);
        void print_state() const;
        const std::string& get_association_id() const { return association_id; };
        const std::string& get_meta_notify_filename() const { return meta_notify_filename; };
    };

    typedef std::map<HANDLE, meta_notify_file*> HANDLE_MAP;
    typedef std::pair<HANDLE, meta_notify_file*> HANDLE_PAIR;
    class handle_study;
    typedef std::map<std::string, handle_study*> STUDY_MAP;
    typedef std::pair<std::string, handle_study*> STUDY_PAIR;

    enum ACTION_TYPE { NO_ACTION = 0, BURN_PER_STUDY, BURN_MULTI, INDEX_INSTANCE };

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

    class handle_dir : public meta_notify_file // handle_dir is association
    {
    private:
        HANDLE handle;
        std::list<std::string> list_file;
        std::set<std::string> set_complete, set_study; // set_study: association[1] -> study[n]
        bool last_find_error, assoc_disconn, disconn_release;
        std::string store_assoc_id, callingAE, callingAddr, calledAE, calledAddr, expected_xfer, auto_publish, last_association_notify_filename;
        unsigned short port;
        
        void process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog);
        NOTIFY_FILE_CONTEXT* process_notify_file(std::istream &ifs, unsigned int file_tag, std::ostream &flog);
        void fill_association_section(NOTIFY_ASSOC_SECTION & nas) const;

    public:
        handle_dir(HANDLE h, const std::string &assoc_id, const std::string &file, const std::string notify_filename, std::ostream *plog)
            : meta_notify_file(assoc_id, file, notify_filename, plog), handle(h), last_find_error(false), last_association_notify_filename(notify_filename),
            assoc_disconn(false), disconn_release(true), port(104) {};
        handle_dir(const handle_dir& o) : meta_notify_file(o), handle(o.handle), last_find_error(o.last_find_error),
            list_file(o.list_file), set_complete(o.set_complete), set_study(o.set_study), port(o.port), last_association_notify_filename(o.last_association_notify_filename),
            store_assoc_id(o.store_assoc_id), callingAE(o.callingAE), callingAddr(o.callingAE), calledAE(o.calledAE), calledAddr(o.calledAddr),
            expected_xfer(o.expected_xfer), assoc_disconn(o.assoc_disconn), disconn_release(o.disconn_release), auto_publish(o.auto_publish) {};
        virtual ~handle_dir();

        handle_dir& operator=(const handle_dir &r);
        void print_state() const;
        HANDLE get_handle() const { return handle; };
        void clean_handle() { handle = NULL; };
        bool insert_study(const std::string &study_uid) { return set_study.insert(study_uid).second; };
        bool insert_complete(const std::string &filename) { return set_complete.insert(filename).second; };
        std::string& get_find_filter(std::string&) const;
        bool is_last_find_error() const { return last_find_error; };
        bool is_association_disconnect() const { return assoc_disconn; };
        bool is_disconnect_release() const { return disconn_release; };
        int file_complete_remain() const { return list_file.size() - set_complete.size(); };
        DWORD find_files(std::ostream &flog, std::function<DWORD(const std::string&)> p);
        DWORD process_notify(const std::string &filename, std::ostream &flog);
        void send_compress_complete_notify(const NOTIFY_FILE_CONTEXT &nfc, handle_study *phdir, std::ostream &flog);
        void broadcast_action_to_all_study(named_pipe_server &nps) const;
        void send_all_compress_ok_notify_and_close_handle(std::ostream &flog);
    };

    class handle_proc : public meta_notify_file
    {
    private:
        HANDLE hlog;
        std::string exec_cmd, exec_name, log_path;
        PROCESS_INFORMATION procinfo;

    public:
        handle_proc(const std::string &assoc_id, const std::string &cwd, const std::string &cmd, const std::string &exec_prog_name, std::ostream *plog) 
            : meta_notify_file(assoc_id, cwd, plog), hlog(NULL), exec_cmd(cmd), exec_name(exec_prog_name)
            { memset(&procinfo, 0, sizeof(PROCESS_INFORMATION)); };
        handle_proc(const handle_proc& o) : meta_notify_file(o), hlog(o.hlog), exec_cmd(o.exec_cmd), exec_name(o.exec_name),
            log_path(o.log_path), procinfo(o.procinfo) {};

        handle_proc& operator=(const handle_proc &r);
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
        handle_compress(const std::string &assoc_id, const std::string &path, const std::string &cmd, const std::string &exec_prog_name, const NOTIFY_FILE_CONTEXT &nfc, std::ostream *plog)
            : handle_proc(assoc_id, path, cmd, exec_prog_name, plog), notify_ctx(nfc) { };

    public:
        static handle_compress* make_handle_compress(const NOTIFY_FILE_CONTEXT &nfc, std::ostream &flog);
        handle_compress(const handle_compress& o) : handle_proc(o), notify_ctx(o.notify_ctx) {};
        handle_compress& operator=(const handle_compress &r);
        void print_state() const;
        NOTIFY_FILE_CONTEXT& get_notify_context() { return notify_ctx; };
    };

    class handle_ris_integration : public handle_proc
    {
    private:
        time_t last_access;

        handle_ris_integration(const std::string &pat_id, const std::string &path, const std::string &cmd, const std::string &exec_prog_name, std::ostream *plog)
            : handle_proc(pat_id, path, cmd, exec_prog_name, plog) { time(&last_access); };
    public:
        static bool make_handle_ris_integration(const std::string &patient, const std::string &prog_path, std::ostream &flog);
        handle_ris_integration(const handle_ris_integration& r) : handle_proc(r), last_access(r.last_access) {};
        handle_ris_integration& operator=(const handle_ris_integration &r);
        void print_state() const;
        const std::string& get_patient_id() const { return get_association_id(); };
        time_t get_last_access() const { return last_access; };
        time_t refresh_last_access() { return time(&last_access); };
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
        std::string study_uid;
        std::string dicomdir_path;
        std::list<action_from_association> list_action;
        std::set<std::string> set_association_path;
        time_t last_idle_time;
        action_from_association last_association_action;

    public:
        handle_study(const std::string &cwd, const std::string &cmd, const std::string &exec_prog_name,
            const std::string &dicomdir, const std::string &study, std::ostream *plog)
            : handle_proc("", cwd, cmd, exec_prog_name, plog), pipe_context(NULL), dicomdir_path(dicomdir), study_uid(study),
            blocked(false), ris_integration_start(false), last_association_action(ACTION_TYPE::INDEX_INSTANCE, cwd, false, plog)
            { time(&last_idle_time); };
        handle_study(const handle_study &r) : handle_proc(r), pipe_context(r.pipe_context), study_uid(r.study_uid),
            dicomdir_path(r.dicomdir_path), set_association_path(r.set_association_path), list_action(r.list_action),
            blocked(r.blocked), ris_integration_start(r.ris_integration_start), last_idle_time(r.last_idle_time),
            last_association_action(r.last_association_action) {};
        
        virtual ~handle_study();
        handle_study& operator=(const handle_study &r);
        void print_state() const;
        void bind_pipe_context(LPPIPEINST p_context) { pipe_context = p_context; };
        DWORD write_message_to_pipe();
        void action_compress_ok(const std::string &filename, const std::string &xfer);
        const std::string& get_study_uid() const { return study_uid; };
        const std::string& get_dicomdir_path() const { return dicomdir_path; };
        const action_from_association& get_last_association_action() const { return last_association_action; };
        const std::set<std::string>& get_set_association_path() const { return set_association_path; };
        bool insert_association_path(const std::string &assoc_path) { return set_association_path.insert(assoc_path).second; };
        //DWORD instance_add_to_dicomdir_ok(const std::string &filename, const std::string &xfer_new, LPOVERLAPPED_COMPLETION_ROUTINE write_complete_callback, std::ostream &flog);
        DWORD append_action(const action_from_association &action);
        bool is_time_out() const;
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
        void check_study_timeout_to_generate_jdf();
    };

    typedef std::map<std::string, MSXML2::IXMLDOMDocument2*> XML_MAP;

    class xml_index : public base_path
    {
    private:
        XML_MAP map_xml;

        MSXML2::IXMLDOMDocument2* create_xmldom(const NOTIFY_FILE_CONTEXT &clc);
        void add_instance(MSXML2::IXMLDOMDocument2 *pXMLDom, const NOTIFY_FILE_CONTEXT &nfc);
        void generate_replace_fields(const std::string &replace_fields_path, MSXML2::IXMLDOMDocument2 *pXMLDom);

    public:
        static xml_index *singleton_ptr;

        xml_index(std::ostream *plog) : base_path("", plog) { if(plog == NULL) plog = &std::cerr; };
        xml_index(const xml_index &r) : base_path(r), map_xml(r.map_xml) {};
        xml_index& operator=(const xml_index &r);
        void make_index(const NOTIFY_FILE_CONTEXT &nfc);
        bool unload_and_sync_study(const std::string &study_uid);
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
