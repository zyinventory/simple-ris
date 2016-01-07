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
    char command[256];
    bool WrittenToPipe;
} CMOVE_LOG_CONTEXT;

#define FILE_ASYN_BUF_SIZE 1024
typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hFileHandle;
	TCHAR chBuff[FILE_ASYN_BUF_SIZE];
    bool fail;
} FILE_OLP_INST, *LPFILE_OLP_INST;

extern bool opt_verbose;
extern int worker_core_num;
extern char pacs_base[MAX_PATH];
extern const char *sessionId;

int process_cmd(const std::string &buf);
void clear_log_context(CMOVE_LOG_CONTEXT *lc = NULL);

#define APC_FUNC_NONE 0
#define APC_FUNC_ReadCommand 1
#define APC_FUNC_Dicomdir 2
#define APC_FUNC_ALL APC_FUNC_ReadCommand | APC_FUNC_Dicomdir

void CALLBACK MakeDicomdir(ULONG_PTR ptr_last_run_apc);
int compress_queue_to_workers(CMOVE_LOG_CONTEXT *lc);

typedef bool (*WORKER_CALLBACK)(void);
bool complete_worker(DWORD wr, HANDLE *objs, WORKER_CALLBACK* cbs, size_t worker_num);
HANDLE *get_worker_handles(size_t *worker_num, size_t *queue_size, WORKER_CALLBACK ** ppCBs = NULL, size_t reserve = 0);

// ------------ Named Pipe ------------

typedef struct {
    HANDLE hProcess, hThread, log;
    char studyUID[65];
} DCMMKDIR_CONTEXT;

typedef struct
{
	OVERLAPPED oOverlap;
	HANDLE hPipeInst;
	TCHAR chRequest[FILE_ASYN_BUF_SIZE];
	DWORD cbRead;
	//TCHAR chReply[FILE_ASYN_BUF_SIZE];
	//DWORD cbToWrite;
    DCMMKDIR_CONTEXT dir_context;
} PIPEINST, *LPPIPEINST;

bool CreateNamedPipeToStaticHandle();
void CloseNamedPipeHandle();
int CreateClientProc(const char *studyUID);

#endif //COMMONLIB_INTERNAL
