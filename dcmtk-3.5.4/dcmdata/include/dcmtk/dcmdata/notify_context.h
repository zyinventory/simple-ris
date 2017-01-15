#pragma once

#ifndef _NOTIFY_CONTEXT_CLASS_
#define _NOTIFY_CONTEXT_CLASS_

#define NOTIFY_ASSOC_ESTA       0x00010010
#define NOTIFY_ASSOC_RELEASE    0xFFFFFFFF
#define NOTIFY_ASSOC_ABORT      0xFFFFFFFD
#define NOTIFY_ACKI_MV_STAT     0xFFFFFFFB
#define NOTIFY_OPER_CANCEL      0xFFFFFFFC
#define NOTIFY_MOVE_STATUS      0x00000900
#define NOTIFY_MV_STAT_REMAIN   0x00001020
#define NOTIFY_MV_STAT_COMPLETE 0x00001021
#define NOTIFY_MV_STAT_FAILED   0x00001022
#define NOTIFY_MV_STAT_WARNING  0x00001023
#define NOTIFY_COMPRESS_OK      0xFFFFFFFA
#define NOTIFY_COMPRESS_FAIL    0xFFFFFFF9

#define NOTIFY_PROC_STOR_START  0xFFFFFFE0

#define INT_TO_CHAR4(pi) reinterpret_cast<const char*>(pi)
#define CHAR4_TO_INT(pc) (*reinterpret_cast<const int*>(pc))
#define STRING_PRE4_TO_INT(pc) (*reinterpret_cast<const int*>(pc.c_str()))

#define NOTIFY_STORE_TAG        "STOR"
#define NOTIFY_MOVE_TAG         "MOVE"
#define NOTIFY_FILE_TAG         "FILE"
#define NOTIFY_MOVE_SESSION_ID  0x00010001
#define NOTIFY_FILE_SEQ_START   0x00011000
#define NOTIFY_ACKN_TAG         "ACKN"
#define NOTIFY_ACKN_ITEM        "ACKI"

#define NOTIFY_LEVEL_FULL       "FULL"
#define NOTIFY_LEVEL_INSTANCE   "INST"
#define NOTIFY_LEVEL_PATIENT    "PATI"
#define NOTIFY_LEVEL_STUDY      "STUD"
#define NOTIFY_LEVEL_SERIES     "SERI"

#define NOTIFY_ALL_COMPRESS_OK  0xFFFFFFF3
#define NOTIFY_ARCHIVE_STUDY    0xFFFFFFF2
#define NOTIFY_ARCHIVE_DICOMDIR 0xFFFFFFF1
#define NOTIFY_XML_OK           0xFFFFFFF0

#define STORE_STATE_DIR "state"

#define MAX_PATH_WIN 260

namespace handle_context
{
    typedef struct _tag_NOTIFY_FILE_CONTEXT_FILE_SECTION {
        long long file_size_receive;
        char filename[MAX_PATH_WIN], hash[12], unique_filename[65], sopClassUID[65],
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
        char id[64], store_assoc_id[64], callingAE[65], callingAddr[65], calledAE[65], calledAddr[65], path[MAX_PATH_WIN], expected_xfer[65], auto_publish[16];
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
}

#endif
