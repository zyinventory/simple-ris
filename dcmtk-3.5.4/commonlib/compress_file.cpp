#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;

bool CMOVE_FILE_SECTION::StorePath(char *buf, size_t buf_size)
{
    if(buf_size < 63) return false;
    uidHash(studyUID, buf, buf_size);
    buf[8] = '\\';
    SeriesInstancePath(seriesUID, instanceUID, buf + 9, buf_size - 9);
    return true;
}

static list<CMOVE_LOG_CONTEXT> workers, queue_compress;

static int create_worker_process(CMOVE_LOG_CONTEXT &lc)
{
    lc.hprocess = INVALID_HANDLE_VALUE;
    lc.hthread = INVALID_HANDLE_VALUE;
    char cmd[1024];
#ifdef _DEBUG
    int mkdir_pos = sprintf_s(cmd, "D:\\zy\\docs\\GitHub\\simple-ris\\dcmtk-3.5.4\\Debug\\dcmcjpeg.exe --encode-jpeg2k-lossless --uid-never %s ", lc.file.filename);
#else
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe --encode-jpeg2k-lossless --uid-never %s ", pacs_base, lc.file.filename);
#endif
    char *mkdir_ptr = cmd + mkdir_pos;
    int ctn = mkdir_pos;
    ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "archdir\\%s\\", lc.file.studyUID);
    lc.file.StorePath(cmd + ctn, sizeof(cmd) - ctn);
    if(!prepareFileDir(mkdir_ptr))
    {
        strerror(errno);
        return 0;
    }
    string logfile("log\\");
    logfile.append(lc.file.instanceUID).append(".txt");
    HANDLE log = CreateFile(logfile.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(log == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("create_worker_process() ", GetLastError());
        return 0;
    }
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdOutput, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdError, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if(0 == CreateProcess(NULL, cmd, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        displayErrorToCerr("create_worker_process() ", GetLastError());
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdError);
        return 0;
    }
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);
    lc.hprocess = pi.hProcess;
    lc.hthread = pi.hThread;
    lc.log = log;
    workers.push_back(lc);
    cerr << "trigger compress "  << lc.file.filename << endl;
    return 1;
}

static void close_log(const CMOVE_LOG_CONTEXT &lc)
{
    if(lc.log && lc.log != INVALID_HANDLE_VALUE)
	{
        ULARGE_INTEGER file_size;
        file_size.QuadPart = 0LL;
        file_size.LowPart = GetFileSize(lc.log, &file_size.HighPart);
        if(file_size.QuadPart == 0LL)
        {
            char filename[MAX_PATH];
            DWORD fnlen = GetFinalPathNameByHandle(lc.log, filename, MAX_PATH, FILE_NAME_NORMALIZED);
            if(fnlen > 0 || fnlen <= MAX_PATH)
            {
                CloseHandle(lc.log);
                if(! DeleteFile(filename)) displayErrorToCerr("close_log() ", GetLastError());
                else if(opt_verbose) cerr << "delete " << filename << " OK" << endl;
            }
        }
    }
}

static char pipeName[64] = "\\\\.\\pipe\\";
static HANDLE hPipeEvent = NULL, hPipe = NULL;
static OVERLAPPED olPipeConnectOnly;

static map<string, list<CMOVE_LOG_CONTEXT> > map_queue_dir;
static map<string, DCMMKDIR_CONTEXT> DirUnboundMap, DirRecycleMap;
static map<string, DWORD> DirUID2PipeInstances;

void CloseNamedPipeHandle()
{
    if(hPipe && hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    hPipe = NULL;
    if(hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE) CloseHandle(hPipeEvent);
    hPipeEvent = NULL;
}

bool CreateNamedPipeToStaticHandle()
{   // manual reset, init signaled, unnamed event object
    hPipeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (hPipeEvent == NULL)
    {
        displayErrorToCerr("CreateEvent()", GetLastError());
        return false;
    }
    memset(&olPipeConnectOnly, 0, sizeof(OVERLAPPED));
    olPipeConnectOnly.hEvent = hPipeEvent;
    strcat_s(pipeName, sessionId);
    hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-type pipe, message read mode, blocking mode
        PIPE_UNLIMITED_INSTANCES, FILE_ASYN_BUF_SIZE, FILE_ASYN_BUF_SIZE, // output, input buffer size 
        10 * 1000, NULL); // client time-out 10s
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("CreateNamedPipe()", GetLastError());
        return false;
    }
    return true;
}

static list<CMOVE_LOG_CONTEXT>& create_or_open_dicomdir_queue(const char *studyUID)
{
    map<string, list<CMOVE_LOG_CONTEXT> >::iterator itlist = map_queue_dir.find(studyUID);
    if(itlist == map_queue_dir.end())
    {
        list<CMOVE_LOG_CONTEXT> lclist;
        map_queue_dir[studyUID] = list<CMOVE_LOG_CONTEXT>();
        return map_queue_dir[studyUID];
    }
    else
        return itlist->second;
}

int CreateClientProc(const char *studyUID)
{
    DCMMKDIR_CONTEXT dc;
    memset(&dc, 0, sizeof(DCMMKDIR_CONTEXT));
    if(!studyUID) studyUID = sessionId;
    strcpy_s(dc.studyUID, studyUID);
    
    create_or_open_dicomdir_queue(studyUID);

#ifdef _DEBUG
    char cmd[1024] = "D:\\zy\\docs\\GitHub\\simple-ris\\dcmtk-3.5.4\\Debug\\dcmmkdir.exe --general-purpose-dvd -A ";
    int mkdir_pos = strlen(cmd);
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", pacs_base);
#endif
    if(strcmp(sessionId, studyUID))
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "+id %s +D %s.dir --viewer GE -pn %s #", dc.studyUID, dc.studyUID, sessionId);
    else
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "+D DICOMDIR --viewer GE -pn %s #", sessionId);

    string logfile(studyUID ? studyUID : "dicomdir");
    logfile.append(".txt");
    HANDLE log = CreateFile(logfile.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(log == INVALID_HANDLE_VALUE)
    {
        DWORD gle = GetLastError();
        displayErrorToCerr("CreateClientProc() ", gle);
        return gle;
    }
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES;
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdOutput, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(GetCurrentProcess(), log, GetCurrentProcess(), &si.hStdError, NULL, TRUE, DUPLICATE_SAME_ACCESS);
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    if(!CreateProcess(NULL, cmd, NULL, NULL, TRUE, NORMAL_PRIORITY_CLASS, NULL, "archdir", &si, &pi))
    {
        DWORD gle = GetLastError();
        DisplayErrorToFileHandle("CreateClientProc()", gle, dc.log);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdError);
        CloseHandle(log);
        return gle;
    }
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    dc.hProcess = pi.hProcess;
    dc.hThread = pi.hThread;
    dc.log = log;

    BOOL fConnected = ConnectNamedPipe(hPipe, &olPipeConnectOnly);
    DWORD gle = GetLastError();
    if(fConnected)
    {
        DisplayErrorToFileHandle("ConnectNamedPipe()", gle, dc.log);
        CloseHandle(dc.hThread);
        CloseHandle(dc.hProcess);
        CloseHandle(dc.log);
        return -1;
    }
    switch(gle)
    {
    case ERROR_IO_PENDING:
        gle = 0;
        break;
    case ERROR_PIPE_CONNECTED:
        if (SetEvent(olPipeConnectOnly.hEvent)) gle = 0;
        else gle = GetLastError();
        break;
    default:
        break;
    }
    if(gle)
    {
        DisplayErrorToFileHandle("ConnectNamedPipe()", gle, dc.log);
        CloseHandle(dc.hThread);
        CloseHandle(dc.hProcess);
        CloseHandle(dc.log);
    }
    else DirUnboundMap[dc.studyUID] = dc;
    return 0;
}

int compress_queue_to_workers(CMOVE_LOG_CONTEXT *plc)
{
    if(plc)
    {
        queue_compress.push_back(*plc);
        cerr << "trigger que_compr "  << plc->file.filename << endl;
    }
    int step = 0;
    while(workers.size() < worker_core_num && queue_compress.size() > 0)
    {
        step += create_worker_process(queue_compress.front());
        queue_compress.pop_front();
    }
    return step;
}

static void DisconnectAndClose(LPPIPEINST lpPipeInst)
{
    if (! DisconnectNamedPipe(lpPipeInst->hPipeInst))
        displayErrorToCerr("DisconnectNamedPipe()", GetLastError());
    CloseHandle(lpPipeInst->hPipeInst);
    map<string, DWORD>::iterator it = DirUID2PipeInstances.end();
    if(strlen(lpPipeInst->dir_context.studyUID))
        it = DirUID2PipeInstances.find(lpPipeInst->dir_context.studyUID);
    if(it == DirUID2PipeInstances.end())
    {
        DWORD ptr = (DWORD)lpPipeInst;
        it = find_if(DirUID2PipeInstances.begin(), DirUID2PipeInstances.end(),
            [ptr](const pair<string, DWORD> p){ return ptr == p.second; });
    }
    if(it != DirUID2PipeInstances.end()) DirUID2PipeInstances.erase(it);
    DirRecycleMap[lpPipeInst->dir_context.studyUID] = lpPipeInst->dir_context;
    if (lpPipeInst != NULL) delete lpPipeInst;
}

static bool writeex_file_to_dcmmkdir(const char *studyUID, size_t *result);

static void CALLBACK writeex_continue(DWORD dwErr, DWORD cbWritten, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST) lpOverLap;
    if (dwErr == 0) writeex_file_to_dcmmkdir(lpPipeInst->dir_context.studyUID, NULL);
}

static bool writeex_file_to_dcmmkdir(const char *studyUID, size_t *result)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST)DirUID2PipeInstances[studyUID];
    if(lpPipeInst == NULL)
    {
        cerr << "writeex_file_to_dcmmkdir(): Pipe instance " << studyUID << " is not exists" << endl;
        return false;
    }

    list<CMOVE_LOG_CONTEXT> &queue_lc = create_or_open_dicomdir_queue(studyUID);
    for(list<CMOVE_LOG_CONTEXT>::iterator it = queue_lc.begin(); it != queue_lc.end(); ++it)
    {
        if(it->WrittenToPipe) continue;

        DWORD cbToWrite = sprintf_s(it->command, "%s|%s\\", it->study.studyUID, it->study.studyUID);
        it->file.StorePath(it->command + cbToWrite, sizeof(it->command) - cbToWrite);
        cbToWrite = strlen(it->command);
        if(WriteFileEx(lpPipeInst->hPipeInst, it->command, cbToWrite, 
            (LPOVERLAPPED) lpPipeInst, (LPOVERLAPPED_COMPLETION_ROUTINE) writeex_continue))
        {
            it->WrittenToPipe = true;
            if(result) result += 1;
        }
        else
        {
            DWORD gle = GetLastError();
            if(gle == ERROR_INVALID_USER_BUFFER || gle == ERROR_NOT_ENOUGH_MEMORY)
                break;
            else
            {
                displayErrorToCerr("writeex_file_to_dcmmkdir()", gle);
                DisconnectAndClose(lpPipeInst);
                return false;
            }
        }
    }
    return true;
}

void CALLBACK MakeDicomdir(ULONG_PTR ptr_last_run_apc)
{
    for(map<string, list<CMOVE_LOG_CONTEXT> >::iterator it = map_queue_dir.begin(); it != map_queue_dir.end(); ++it)
    {
        const string &studyUID = it->first;
        writeex_file_to_dcmmkdir(studyUID.c_str(), NULL);
    }
    if(ptr_last_run_apc) *(int*)ptr_last_run_apc &= ~APC_FUNC_Dicomdir;
}

static bool check_reading_message(LPPIPEINST lpPipeInst, DWORD cbBytesRead, string *studyUID, string *afterSperator)
{
    char *sp = strchr(lpPipeInst->chRequest, '|');
    if(sp == NULL) return false;
    bool releaseString = false;
    if(studyUID == NULL)
    {
        studyUID = new string();
        releaseString = true;
    }

    studyUID->append(lpPipeInst->chRequest, sp - lpPipeInst->chRequest);
    if(afterSperator)
    {
        size_t otherLen = cbBytesRead - studyUID->length() - 1;
        if(otherLen > 0) afterSperator->append(++sp, otherLen);
    }
    if(studyUID->compare(lpPipeInst->dir_context.studyUID))
    {
        if(releaseString) delete studyUID;
        return false;
    }
    else
    {
        if(releaseString) delete studyUID;
        return true;
    }
}

static void CALLBACK ReadPipeComplete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST) lpOverLap;
    BOOL fRead = FALSE;

    // The read operation has finished, so write a response (if no error occurred).
    if ((dwErr == 0) && (cbBytesRead != 0))
    {
        string studyUID, instanceUID;
        if(!check_reading_message(lpPipeInst, cbBytesRead, &studyUID, &instanceUID))
            goto pipe_read_complete_test;

        // erase instance which is written to dicomdir
        list<CMOVE_LOG_CONTEXT> &queue_lc = create_or_open_dicomdir_queue(studyUID.c_str());
        list<CMOVE_LOG_CONTEXT>::iterator it = find_if(queue_lc.begin(), queue_lc.end(),
            [&instanceUID](CMOVE_LOG_CONTEXT &lc){ return instanceUID.compare(lc.file.instanceUID) == 0; });
        if(it != queue_lc.end()) queue_lc.erase(it);

        // next read loop, until dcmmkdir close pipe
        fRead = ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chRequest,
            FILE_ASYN_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)ReadPipeComplete);
    }
pipe_read_complete_test:
    // Disconnect if an error occurred.
    if (! fRead) DisconnectAndClose(lpPipeInst);
}

static void CALLBACK FirstReadBindPipe(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;

    BOOL bindOK = FALSE;
    if ((dwErr == 0) && (cbBytesRead != 0))
    {
        string studyUID;
        if(!check_reading_message(lpPipeInst, cbBytesRead, &studyUID, NULL))
            goto pipe_bind_complete_test;

        map<string, DCMMKDIR_CONTEXT>::iterator itTemp = DirUnboundMap.find(studyUID);
        if(itTemp == DirUnboundMap.end())
        {
            cerr << "can't bind incoming request from unbound queue: " << studyUID.c_str() << endl;
            goto pipe_bind_complete_test;
        }
        bindOK = true;
        lpPipeInst->dir_context = itTemp->second;
        map<string, DWORD>::iterator itPipe = DirUID2PipeInstances.find(studyUID);
        if(itPipe == DirUID2PipeInstances.end())
            DirUID2PipeInstances[studyUID] = (DWORD)lpPipeInst;
        DirUnboundMap.erase(itTemp);

        bool result = ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chRequest,
            FILE_ASYN_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)ReadPipeComplete);
    
        if (!result) DisconnectAndClose(lpPipeInst);

        // create new pipe server
        bool createOK = false;
        // for each map_queue_dir->list
        for(map<string, list<CMOVE_LOG_CONTEXT> >::iterator pairlist = map_queue_dir.begin(); pairlist != map_queue_dir.end(); ++pairlist)
        {
            const string &studyUID = pairlist->first;
            if(!pairlist->second.empty())
            {
                LPPIPEINST lpPipeInst = (LPPIPEINST)DirUID2PipeInstances[studyUID];
                if(lpPipeInst == NULL) // no corresponding pipe instance, create new
                {
                    createOK = CreateNamedPipeToStaticHandle();
                    if(createOK) createOK = CreateClientProc(studyUID.c_str());
                    if(createOK) break;
                    else CloseNamedPipeHandle();
                }
            }
        }
    }
pipe_bind_complete_test:
    if (! bindOK) DisconnectAndClose(lpPipeInst);
}

static bool PipeConnected()
{
    DWORD dwWait, cbRet;
    if(!GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, FALSE))
    {
        DWORD gle = GetLastError();
        displayErrorToCerr("GetOverlappedResult()", gle);
        return false;
    }
    LPPIPEINST lpPipeInst = new PIPEINST;
    memset(lpPipeInst, 0, sizeof(PIPEINST));
    lpPipeInst->hPipeInst = hPipe;
    bool result = ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chRequest,
            FILE_ASYN_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)FirstReadBindPipe);
    
    if (!result) DisconnectAndClose(lpPipeInst);
    return result;
}

bool complete_worker(DWORD wr, HANDLE *objs, WORKER_CALLBACK* cbs, size_t worker_num)
{
    CMOVE_LOG_CONTEXT over_lc;
    wr -= WAIT_OBJECT_0;

    if(cbs && cbs[wr]) return cbs[wr]();

    HANDLE over_hProcess = objs[wr];
    list<CMOVE_LOG_CONTEXT>::iterator it = find_if(workers.begin(), workers.end(), 
        [over_hProcess](CMOVE_LOG_CONTEXT &clc) { return clc.hprocess == over_hProcess; });
    over_lc = *it;
    workers.erase(it);
    cerr << "trigger complete " << over_lc.file.filename << endl;
    bool result = false;
    if(over_lc.hprocess != INVALID_HANDLE_VALUE)
    {
        CloseHandle(over_lc.hthread);
        over_lc.hthread = INVALID_HANDLE_VALUE;
        CloseHandle(over_lc.hprocess);
        over_lc.hprocess = INVALID_HANDLE_VALUE;

        // todo: shall close it after index
        close_log(over_lc);

        create_or_open_dicomdir_queue(".").push_back(over_lc);
        create_or_open_dicomdir_queue(over_lc.study.studyUID).push_back(over_lc);
    }
    compress_queue_to_workers(NULL);
    return result;
}

HANDLE *get_worker_handles(size_t *worker_num, size_t *queue_size, WORKER_CALLBACK ** ppCBs, HANDLE hDir)
{
    if(queue_size) *queue_size = queue_compress.size();
    size_t wk_num = workers.size();
    bool hasPipeEvent = (hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE);
    if(hasPipeEvent) ++wk_num;
    if(hDir && hDir != INVALID_HANDLE_VALUE) ++wk_num;
    if(wk_num > 0)
    {
        HANDLE *objs = new HANDLE[wk_num];
        if(ppCBs) *ppCBs = new WORKER_CALLBACK[wk_num];
        memset(*ppCBs, 0, sizeof(WORKER_CALLBACK) * wk_num);
        int i = 0;
        if(hasPipeEvent)
        {
            objs[i] = hPipeEvent;
            *ppCBs[i++] = PipeConnected;
        }
        if(hDir && hDir != INVALID_HANDLE_VALUE)
        {
            objs[i] = hDir;
            *ppCBs[i++] = read_cmd_continous;
        }
        transform(workers.begin(), workers.end(), objs + i, 
            [](const CMOVE_LOG_CONTEXT &clc) { return clc.hprocess; });
        if(worker_num) *worker_num = wk_num;
        return objs;
    }
    else
    {
        if(worker_num) *worker_num = 0;
        if(ppCBs) *ppCBs = NULL;
        return NULL;
    }
}
