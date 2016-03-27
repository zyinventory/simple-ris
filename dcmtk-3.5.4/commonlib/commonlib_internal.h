#ifndef COMMONLIB_INTERNAL
#define COMMONLIB_INTERNAL

#define TYPE_MOVE       'M'
#define TYPE_ASSOC      'T'
#define TYPE_FILE       'F'
#define TYPE_PATIENT    'P'
#define TYPE_STUDY      'S'
#define TYPE_SERIES     'E'
#define TYPE_INSTANCE   'I'
#define TYPE_NOTIFY     'N'
#define TYPE_ACK        'A'

#define ASSOC_ESTA  0x00010010
#define FILE_START  0x00011000
#define ASSOC_TERM  0xFFFFFFFF
#define ASSOC_ABORT 0xFFFFFFFD

#define NOTIFY_START    0xFFFFFFFB
#define NOTIFY_REMAIN   0x00001020
#define NOTIFY_COMPLETE 0x00001021
#define NOTIFY_FAILED   0x00001022
#define NOTIFY_WARNING  0x00001023
#define NOTIFY_END      0xFFFFFFFE
#define NOTIFY_CANCEL   0xFFFFFFFC
#define NOTIFY_COMPR_OK 0xFFFFFFFA
#define NOTIFY_ARCHIVE  0xFFFFFFF9
#define NOTIFY_DICOMDIR 0xFFFFFFF8
#define NOTIFY_XML_OK   0xFFFFFFF0

FILE *create_transaction_append_file(const char *fn);

typedef struct {
    char id[32], callingAE[65], callingAddr[65], calledAE[65], calledAddr[65];
    unsigned short port;
} CMOVE_ASSOC_SECTION;

typedef struct {
    unsigned int tag;
    char filename[MAX_PATH], hash[12], unique_filename[65], patientID[65], studyUID[65], seriesUID[65], instanceUID[65], xfer[16], xfer_new[16];
    bool inFile, isEncapsulated;
    const char* StorePath(char sp = '/');
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
    HANDLE hprocess, hthread, log;
    CMOVE_ASSOC_SECTION assoc;
    CMOVE_FILE_SECTION file;
    CMOVE_PATIENT_SECTION patient;
    CMOVE_STUDY_SECTION study;
    CMOVE_SERIES_SECTION series;
} CMOVE_LOG_CONTEXT;

#define FILE_ASYN_BUF_SIZE 1024

extern bool opt_verbose, ready_to_close_dcmmkdir_workers;
extern size_t worker_core_num;
extern char pacs_base[MAX_PATH];
extern const char *sessionId;
extern HANDLE hDirNotify;

int process_cmd(const std::string &buf);
void clear_log_context(CMOVE_LOG_CONTEXT *lc = NULL);
int compress_queue_to_workers(CMOVE_LOG_CONTEXT *lc);
bool is_idle(const char *studyUID = NULL);
void close_all_blocked_pipe_instances();

typedef DWORD (*WORKER_CALLBACK)(HANDLE);
DWORD worker_complete(DWORD wr, HANDLE *objs, WORKER_CALLBACK* cbs, size_t worker_num);
HANDLE *get_worker_handles(size_t *worker_num, size_t *queue_size, WORKER_CALLBACK ** ppCBs = NULL, size_t reserve = 0);

// ------------ Named Pipe ------------

typedef struct {
    HANDLE hProcess, hThread, log;
    bool detachPipeInstance;
    char dot_or_study_uid[65];
} DCMMKDIR_CONTEXT;

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	TCHAR chBuffer[FILE_ASYN_BUF_SIZE];
	DWORD cbShouldWrite;
    char dot_or_study_uid[65];
} PIPEINST, *LPPIPEINST;

DWORD NamedPipe_CreateListening(const char *pipe_name = NULL, bool wait = false);
void  NamedPipe_CloseHandle(bool close_event = false);
DWORD NamedPipe_CreateClientProc(const char *dot_or_study_uid);

// ------------ Make Index ------------

errno_t make_index(const CMOVE_LOG_CONTEXT &clc);
void save_index_study_receive_to_session();
void merge_index_study_patient_date(const char *pacs_base, bool overwrite, std::map<std::string, LARGE_INTEGER> &map_move_study_status);

#endif //COMMONLIB_INTERNAL
