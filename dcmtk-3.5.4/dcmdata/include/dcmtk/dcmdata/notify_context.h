#pragma once

#ifndef _NOTIFY_CONTEXT_CLASS_
#define _NOTIFY_CONTEXT_CLASS_

#include <Windows.h>
#include <time.h>
#include <iostream>
#include <list>

namespace handle_context
{
    typedef struct _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION {
        long long file_size_receive;
        char filename[MAX_PATH], hash[12], unique_filename[65], sopClassUID[65],
            patientID[65], studyUID[65], seriesUID[65], instanceUID[65],
            xfer[16], xfer_new[16], charset[65];
        long number;
        bool isEncapsulated;
        char PathSeparator() const { return unique_filename[8]; }
    } NOTIFY_FILE_CONTEXT_FILE_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_PATIENT_SECTION {
        char patientID[65], patientsName[65], birthday[19], height[17], weight[17], sex[3];
    } NOTIFY_FILE_CONTEXT_PATIENT_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_STUDY_SECTION {
        char studyUID[65], studyDate[19], studyTime[17], accessionNumber[65], studyID[17];
    } NOTIFY_FILE_CONTEXT_STUDY_SECTION;

    typedef struct _tag_NOTIFY_FILE_CONTEXT_SERIES_SECTION {
        char seriesUID[65], modality[17];
        long number;
    } NOTIFY_FILE_CONTEXT_SERIES_SECTION;

    typedef struct _tag_NOTIFY_ASSOC_SECTION {
        char id[64], store_assoc_id[64], callingAE[65], callingAddr[65], calledAE[65], calledAddr[65], path[MAX_PATH], expected_xfer[65], auto_publish[16];
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
        bool operator<(const struct _tag_NOTIFY_FILE_CONTEXT &r) const
        {
            int cmp = strcmp(src_notify_filename, r.src_notify_filename);
            if(cmp < 0) return true;
            else if(cmp > 0) return false;
            else
            {
                cmp = strcmp(file.unique_filename, r.file.unique_filename);
                if(cmp < 0) return true;
                else if(cmp > 0) return false;
                else
                {
                    cmp = strcmp(file.filename, r.file.filename);
                    if(cmp < 0) return true;
                    else return false;
                }
            }
        };
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
        void set_path(const std::string& new_path) { path = new_path; };

    public:
        base_path(const base_path &r) : path(r.path), pflog(r.pflog) {};
        base_path& operator=(const base_path &r) { path = r.path; pflog = r.pflog; return *this; };
        virtual void print_state() const { *pflog << "base_path::print_state() path: " << path << std::endl; };
        virtual HANDLE get_handle() const { return NULL; };
        const std::string& get_path() const { return path; };
        std::ostream* get_err_stream() const { return pflog; };
    };

    class base_dir : public base_path
    {
    private:
        std::string id, meta_notify_filename;
        time_t last_access;
        int timeout;

    protected:
        base_dir(const std::string &assoc_id, const std::string &path, const std::string &filename, int time_out_diff, std::ostream *plog)
            : base_path(path, plog), id(assoc_id), meta_notify_filename(filename), timeout(time_out_diff) { time(&last_access); };
        void set_id(const std::string &new_id) { id = new_id; };
        void set_meta_notify_filename(const std::string &new_meta) { meta_notify_filename = new_meta; };

    public:
        base_dir(const base_dir& r) : base_path(r), id(r.id),
            meta_notify_filename(r.meta_notify_filename),  last_access(r.last_access) {};
        base_dir& operator=(const base_dir &r)
        {
            base_path::operator=(r);
            id = r.id;
            meta_notify_filename = r.meta_notify_filename;
            last_access = r.last_access;
            return *this;
        };
        void print_state() const
        {
            *pflog << "base_dir::print_state() id: " << id << std::endl
                << "\tmeta_notify_filename: " << meta_notify_filename << std::endl
                << "\tlast_access: " << ctime(&last_access); // ctime() shall term with LF
            base_path::print_state();
        };
        const std::string& get_id() const { return id; };
        const std::string& get_meta_notify_filename() const { return meta_notify_filename; };
        int get_timeout() const { return timeout; };
        time_t get_last_access() const { return last_access; };
        time_t refresh_last_access() { return time(&last_access); };
        virtual bool is_time_out() const
        {
            if(timeout)
            {
                time_t diff = 0LL, last_access = get_last_access();
                time(&diff);
                diff -= last_access;
                if(diff > timeout) return true;
            }
            return false;
        };
    };
}

#endif
