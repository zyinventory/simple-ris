#pragma once

#ifndef _NOTIFY_CONTEXT_CLASS_
#define _NOTIFY_CONTEXT_CLASS_

#include <iostream>

namespace handle_context
{
    typedef struct _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION {
        long long file_size_receive;
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
        bool operator<(const struct _tag_NOTIFY_FILE_CONTEXT &r) const;
        bool operator==(const struct _tag_NOTIFY_FILE_CONTEXT &r) const { return (!(*this < r) && !(r < *this)); };
    } NOTIFY_FILE_CONTEXT;

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
        virtual void print_state() const { *pflog << "base_path::print_state() path: " << path << std::endl; };
        virtual HANDLE get_handle() const { return NULL; };
        const std::string& get_path() const { return path; };
    };
}

#endif
