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
}

#endif
