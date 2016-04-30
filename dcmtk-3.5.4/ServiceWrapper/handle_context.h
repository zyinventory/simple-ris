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

    public:
        notify_file(const std::string &assoc_id, const std::string &file) : association_id(assoc_id), path(file) {};
        notify_file(const notify_file& o) : association_id(o.association_id), path(o.path) {};

        notify_file& operator=(const notify_file &r);

        virtual HANDLE get_handle() const { return NULL; };
        const std::string& get_association_id() const { return association_id; };
        const std::string& get_path() const { return path; };
    };

    class handle_dir : public notify_file
    {
    private:
        HANDLE handle;
        std::list<std::string> filelist;
        bool last_find_error, assoc_disconn, disconn_release;
        std::string store_assoc_id, callingAE, callingAddr, calledAE, calledAddr, expected_xfer;
        unsigned short port;
        
        void process_notify_association(std::istream &ifs, unsigned int tag, std::ostream &flog);

    public:
        handle_dir(HANDLE h, const std::string &assoc_id, const std::string &file)
            : notify_file(assoc_id, file), handle(h), last_find_error(false), 
            assoc_disconn(false), disconn_release(true), port(104) {};

        handle_dir(const handle_dir& o) : notify_file(o), handle(o.handle), 
            last_find_error(o.last_find_error), filelist(o.filelist), port(o.port), store_assoc_id(o.store_assoc_id),
            callingAE(o.callingAE), callingAddr(o.callingAE), calledAE(o.calledAE), calledAddr(o.calledAddr),
            expected_xfer(o.expected_xfer), assoc_disconn(o.assoc_disconn), disconn_release(o.disconn_release){};

        handle_dir& operator=(const handle_dir &r);
        virtual ~handle_dir() { FindCloseChangeNotification(handle); };

        HANDLE get_handle() const { return handle; };
        std::string& get_find_filter(std::string&) const;
        bool is_last_find_error() const { return last_find_error; };
        DWORD find_files(std::ostream &flog, std::function<DWORD(const std::string&)> p);
        DWORD process_notify(const std::string &filename, std::ostream &flog);
        handle_context::CMOVE_NOTIFY_CONTEXT* process_notify_file(std::istream &ifs, unsigned int file_tag, std::ostream &flog);
    };

    class handle_proc : public notify_file
    {
    private:
        std::string exec_cmd;
        PROCESS_INFORMATION procinfo;

    public:
        handle_proc(const std::string &assoc_id, const std::string &file, const std::string &cmd) 
            : notify_file(assoc_id, file), exec_cmd(cmd) { memset(&procinfo, 0, sizeof(PROCESS_INFORMATION)); };
        handle_proc(const handle_proc& o) : notify_file(o), exec_cmd(o.exec_cmd), procinfo(o.procinfo) {};

        handle_proc& operator=(const handle_proc &r);
        virtual ~handle_proc() { CloseHandle(procinfo.hThread); CloseHandle(procinfo.hProcess); };

        HANDLE get_handle() const { return procinfo.hProcess; };
        const std::string& get_exec_cmd() const { return exec_cmd; };
        const PROCESS_INFORMATION& get_procinfo() const { return procinfo; };
        int create_process(const char *exec_name, std::ostream &flog);
    };
}
#endif
