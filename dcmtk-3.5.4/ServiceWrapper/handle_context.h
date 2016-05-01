#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

#include <string>

namespace handle_context
{
    typedef struct {
        char filename[MAX_PATH], hash[12], unique_filename[65], patientID[65], studyUID[65], seriesUID[65], instanceUID[65], xfer[16], xfer_new[16];
        bool isEncapsulated;
        const char* StorePath(char sp = '\\');
        char PathSeparator() const { return unique_filename[8]; }
    } CMOVE_FILE_SECTION;

    typedef struct {
        char patientID[65], patientsName[65], birthday[9], height[10], weight[10], sex[3];
    } CMOVE_PATIENT_SECTION;

    typedef struct {
        char studyUID[65], studyDate[9], studyTime[15], accessionNumber[65];
    } CMOVE_STUDY_SECTION;

    typedef struct {
        char seriesUID[65], modality[17];
    } CMOVE_SERIES_SECTION;

    typedef struct {
        char association_id[64];
        unsigned int file_seq;
        CMOVE_FILE_SECTION file;
        CMOVE_PATIENT_SECTION patient;
        CMOVE_STUDY_SECTION study;
        CMOVE_SERIES_SECTION series;
    } CMOVE_NOTIFY_CONTEXT;

    class notify_file
    {
    private:
        std::string association_id, path;

    protected:
        notify_file(const std::string &assoc_id, const std::string &file) : association_id(assoc_id), path(file) {};

    public:
        notify_file(const notify_file& o) : association_id(o.association_id), path(o.path) {};

        notify_file& operator=(const notify_file &r);

        virtual HANDLE get_handle() const { return NULL; };
        const std::string& get_association_id() const { return association_id; };
        const std::string& get_path() const { return path; };
    };

    class handle_dir : public notify_file // handle_dir is association
    {
    private:
        HANDLE handle;
        std::list<std::string> list_file;
        std::set<std::string> set_study; // association[1] -> study[n]
        bool last_find_error, assoc_disconn, disconn_release;
        std::string store_assoc_id, callingAE, callingAddr, calledAE, calledAddr, expected_xfer;
        unsigned short port;
        
        void process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog);
        handle_context::CMOVE_NOTIFY_CONTEXT* process_notify_file(std::istream &ifs, unsigned int file_tag, std::ostream &flog);

    public:
        handle_dir(HANDLE h, const std::string &assoc_id, const std::string &file)
            : notify_file(assoc_id, file), handle(h), last_find_error(false), 
            assoc_disconn(false), disconn_release(true), port(104) {};

        handle_dir(const handle_dir& o) : notify_file(o), handle(o.handle), last_find_error(o.last_find_error),
            list_file(o.list_file), set_study(o.set_study), port(o.port), store_assoc_id(o.store_assoc_id),
            callingAE(o.callingAE), callingAddr(o.callingAE), calledAE(o.calledAE), calledAddr(o.calledAddr),
            expected_xfer(o.expected_xfer), assoc_disconn(o.assoc_disconn), disconn_release(o.disconn_release){};

        handle_dir& operator=(const handle_dir &r);
        virtual ~handle_dir() { FindCloseChangeNotification(handle); };
        
        HANDLE get_handle() const { return handle; };
        bool insert_study(const std::string &study_uid) { return set_study.insert(study_uid).second; };
        std::string& get_find_filter(std::string&) const;
        bool is_last_find_error() const { return last_find_error; };
        DWORD find_files(std::ostream &flog, std::function<DWORD(const std::string&)> p);
        DWORD process_notify(const std::string &filename, std::ostream &flog);
    };

    class handle_proc : public notify_file
    {
    private:
        HANDLE hlog;
        std::string exec_cmd, exec_name, log_path;
        PROCESS_INFORMATION procinfo;

    public:
        handle_proc(const std::string &assoc_id, const std::string &cwd, const std::string &cmd, const std::string &exec_prog_name) 
            : notify_file(assoc_id, cwd), hlog(NULL), exec_cmd(cmd), exec_name(exec_prog_name) { memset(&procinfo, 0, sizeof(PROCESS_INFORMATION)); };
        handle_proc(const handle_proc& o) : notify_file(o), hlog(o.hlog), exec_cmd(o.exec_cmd), exec_name(o.exec_name),
            log_path(o.log_path), procinfo(o.procinfo) {};

        handle_proc& operator=(const handle_proc &r);
        virtual ~handle_proc();

        HANDLE get_handle() const { return procinfo.hProcess; };
        const std::string& get_exec_cmd() const { return exec_cmd; };
        const PROCESS_INFORMATION& get_procinfo() const { return procinfo; };
        int start_process(std::ostream &flog);
    };

    class handle_compress : public handle_proc
    {
    private:
        CMOVE_NOTIFY_CONTEXT notify_ctx;

    protected:
        handle_compress(const std::string &assoc_id, const std::string &cwd, const std::string &cmd, 
            const std::string &exec_prog_name, const CMOVE_NOTIFY_CONTEXT &cnc)
            : handle_proc(assoc_id, cwd, cmd, exec_prog_name), notify_ctx(cnc) { };

    public:
        static handle_compress* make_handle_compress(const CMOVE_NOTIFY_CONTEXT &cnc, std::map<HANDLE, handle_context::notify_file*> &map_handle);
        handle_compress(const handle_compress& o) : handle_proc(o), notify_ctx(o.notify_ctx) {};
        handle_compress& operator=(const handle_compress &r);
        CMOVE_NOTIFY_CONTEXT& get_notify_context() { return notify_ctx; };
    };

    class handle_dicomdir : public handle_proc
    {
    private:
        std::string study_uid, dicomdir_path;
        std::set<std::string> set_association_path; // study[1] -> association[n]

    protected:
        handle_dicomdir(const std::string &assoc_id, const std::string &cwd, const std::string &cmd, 
            const std::string &exec_prog_name, const std::string &dicomdir, const std::string &study)
            : handle_proc(assoc_id, cwd, cmd, exec_prog_name), dicomdir_path(dicomdir), study_uid(study) {};
    public:
        static handle_dicomdir* make_handle_dicomdir(const std::string &study);
        handle_dicomdir(const handle_dicomdir &r) : handle_proc(r), study_uid(r.study_uid), 
            dicomdir_path(r.dicomdir_path), set_association_path(r.set_association_path) {};
        
        handle_dicomdir& operator=(const handle_dicomdir &r);

        const std::string& get_study_uid() const { return study_uid; };
        const std::string& get_dicomdir_path() const { return dicomdir_path; };
        bool insert_association_path(const std::string &assoc_path) { return set_association_path.insert(assoc_path).second; };
    };
}
#endif
