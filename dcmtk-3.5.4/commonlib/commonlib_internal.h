#ifndef COMMONLIB_INTERNAL
#define COMMONLIB_INTERNAL
typedef struct {
    char callingAE[65], callingAddr[40], calledAE[65], calledAddr[40];
    unsigned short port;
} CMOVE_ASSOC_SECTION;

typedef struct {
    unsigned int tag;
    char filename[MAX_PATH], patientID[65], studyUID[65], seriesUID[65], instanceUID[65], xfer[16];
    bool inFile, isEncapsulated;
    bool StorePath(char *buf, size_t buf_size);
} CMOVE_FILE_SECTION;

typedef struct {
    char patientID[65], patientsName[65], birthday[9], height[10], weight[10], sex;
} CMOVE_PATIENT_SECTION;

typedef struct {
    char studyUID[65], studyDate[9], studyTime[15], accessionNumber[65];
} CMOVE_STUDY_SECTION;

typedef struct {
    char seriesUID[65], modality[17];
} CMOVE_SERIES_SECTION;

typedef struct {
    HANDLE hprocess, hthread, log;
    CMOVE_ASSOC_SECTION assoc;
    CMOVE_FILE_SECTION file;
    CMOVE_PATIENT_SECTION patient;
    CMOVE_STUDY_SECTION study;
    CMOVE_SERIES_SECTION series;
} CMOVE_LOG_CONTEXT;

extern bool opt_verbose;
extern int worker_core_num;
extern char pacs_base[MAX_PATH];

bool commit_file_to_workers(CMOVE_LOG_CONTEXT *lc);
int process_cmd(const char *buf);
void clear_log_context(CMOVE_LOG_CONTEXT *lc = NULL);

#endif //COMMONLIB_INTERNAL
