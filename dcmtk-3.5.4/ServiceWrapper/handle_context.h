#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

#include <string>

namespace handle_context
{
    //enum handle_type { DIR, QRSCP, COMPRESS, ASSOC, STUDY };
    typedef struct {
        unsigned int tag;
        char association_id[64], filename[MAX_PATH], hash[12], unique_filename[65], studyUID[65], seriesUID[65], instanceUID[65];
        const char* StorePath(char sp = '/');
        char PathSeparator() const { return unique_filename[8]; }
    } CMOVE_FILE_SECTION;

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
        bool last_find_error;

    public:
        handle_dir(HANDLE h, const std::string &assoc_id, const std::string &file)
            : notify_file(assoc_id, file), handle(h), last_find_error(false) {};
        handle_dir(const handle_dir& o) : notify_file(o), handle(o.handle), 
            last_find_error(o.last_find_error), filelist(o.filelist) {};

        handle_dir& operator=(const handle_dir &r);
        virtual ~handle_dir() { FindCloseChangeNotification(handle); };

        HANDLE get_handle() const { return handle; };
        std::string& get_find_filter(std::string&) const;
        bool is_last_find_error() const { return last_find_error; };
        DWORD find_files(std::ostream &flog, std::function<DWORD(const std::string&)> p);
        DWORD process_notify_file(const std::string &filename, std::ostream &flog);
        void process_file_notify_file(std::ifstream &ifs, unsigned int file_tag);
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
