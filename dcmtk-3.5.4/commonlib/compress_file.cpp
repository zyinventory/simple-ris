#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;

void x_www_form_codec_encode(const char *data, ostream *ostrm);

const char* CMOVE_FILE_SECTION::StorePath(char sp)
{
    HashStr(studyUID, unique_filename, sizeof(unique_filename));
    unique_filename[8] = sp;
    SeriesInstancePath(seriesUID, instanceUID, unique_filename + 9, sizeof(unique_filename) - 9, sp);
    sprintf_s(hash, "%c%c%c%c%c%c%c%c%c%c%c",
        unique_filename[0], unique_filename[1], sp, unique_filename[2], unique_filename[3], sp, 
        unique_filename[4], unique_filename[5], sp, unique_filename[6], unique_filename[7]);
    return unique_filename;
}

static list<CMOVE_LOG_CONTEXT> workers, queue_compress;

static int create_worker_process(CMOVE_LOG_CONTEXT &lc)
{
    lc.hprocess = INVALID_HANDLE_VALUE;
    lc.hthread = INVALID_HANDLE_VALUE;
    const char *verbose_flag = opt_verbose ? "-v" : "";
#ifdef _DEBUG
    int mkdir_pos = 0;
    char cmd[1024] = __FILE__;
    char *p = strrchr(cmd, '\\');
    if(p)
    {
        ++p;
        mkdir_pos = p - cmd;
        mkdir_pos += sprintf_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", verbose_flag, lc.file.filename);
    }
    else
        mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", pacs_base, verbose_flag, lc.file.filename);
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmcjpeg.exe %s --encode-jpeg2k-lossless --uid-never %s ", pacs_base, verbose_flag, lc.file.filename);
#endif
    char *mkdir_ptr = cmd + mkdir_pos;
    int ctn = mkdir_pos;
    ctn += sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "archdir\\%s\\", lc.file.studyUID);
    if(strlen(lc.file.unique_filename) == 0) lc.file.StorePath('\\');
    strcpy_s(cmd + ctn, sizeof(cmd) - ctn, lc.file.unique_filename);
    if(!PrepareFileDir(mkdir_ptr))
    {
        cerr << __FUNCSIG__ " PrepareFileDir() error" << endl;
        return 0;
    }
    string logfile(lc.file.instanceUID);
    logfile.append(".txt");
    HANDLE log = CreateFile(logfile.c_str(), GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_NORMAL, NULL);
    if(log == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr(__FUNCSIG__ " CreateFile()", GetLastError());
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
    /*
    DWORD creationFlags = IDLE_PRIORITY_CLASS;
#ifdef _DEBUG
    creationFlags |= DEBUG_PROCESS;
#endif
    */
    if(0 == CreateProcess(NULL, cmd, NULL, NULL, TRUE, IDLE_PRIORITY_CLASS, NULL, NULL, &si, &pi))
    {
        displayErrorToCerr(__FUNCSIG__ " CreateProcess()", GetLastError());
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
    cout << "trigger compress "  << lc.file.filename << endl;
    return 1;
}

static void close_log(HANDLE log)
{
    if(log && log != INVALID_HANDLE_VALUE)
	{
        DWORD cbRead = 0;
        char *buff = new char[4096];
        HANDLE herr = INVALID_HANDLE_VALUE;
        DWORD gle = 0;
        if(0 == SetFilePointer(log, 0, NULL, FILE_BEGIN))
        {
            while(ReadFile(log, buff, 4096, &cbRead, NULL) && cbRead)
            {
                if(herr == INVALID_HANDLE_VALUE)
                {
                    herr = CreateFile("cmove_error.txt",  FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                    if(herr == INVALID_HANDLE_VALUE)
                    {
                        gle = GetLastError();
                        break;
                    }
                }
                DWORD written = 0;
                if(!WriteFile(herr, buff, cbRead, &written, NULL))
                {
                    gle = GetLastError();
                    break;
                }
            }
        }
        else gle = GetLastError();
        delete[] buff;
        if(gle)
            displayErrorToCerr(__FUNCSIG__, gle);
        if(herr != INVALID_HANDLE_VALUE) CloseHandle(herr);
        CloseHandle(log);
    }
}

static map<string, list<CMOVE_LOG_CONTEXT> > map_dir_queue_list;
static list<DCMMKDIR_CONTEXT> list_dir_workers;
static list<LPPIPEINST> list_blocked_pipe_instances;

static DWORD close_dcmmkdir_worker(HANDLE hProc)
{
    list<DCMMKDIR_CONTEXT>::iterator it = find_if(list_dir_workers.begin(), list_dir_workers.end(),
        [hProc](DCMMKDIR_CONTEXT &dc) { return hProc == dc.hProcess; });
    if(it != list_dir_workers.end())
    {
        if(opt_verbose) cout << __FUNCSIG__ " : close dcmmkdir process " << it->hProcess << ", study uid " << it->dot_or_study_uid << endl;
        CloseHandle(it->hThread);
        CloseHandle(it->hProcess);
        close_log(it->log);
        list_dir_workers.erase(it);
    }
    else
        cerr << __FUNCSIG__ " : dcmmkdir process " << it->hProcess << " not found, study uid " << it->dot_or_study_uid << endl;
    return ERROR_SUCCESS;
}

// "." is dicomdir for all study
static map<string, list<CMOVE_LOG_CONTEXT> >::iterator create_or_open_dicomdir_queue(const char *dot_or_study_uid)
{
    if(dot_or_study_uid == NULL || strlen(dot_or_study_uid) == 0)
    {
        cerr << __FUNCSIG__ " : Study UID must be not NULL" << endl;
        return map_dir_queue_list.end();
    }
    map<string, list<CMOVE_LOG_CONTEXT> >::iterator itlist = map_dir_queue_list.find(dot_or_study_uid);
    if(itlist == map_dir_queue_list.end())
    {
        list<CMOVE_LOG_CONTEXT> lclist;
        map_dir_queue_list[dot_or_study_uid] = list<CMOVE_LOG_CONTEXT>();
        itlist = map_dir_queue_list.find(dot_or_study_uid);
    }
    if(list_dir_workers.end() == find_if(list_dir_workers.begin(), list_dir_workers.end(), 
        [dot_or_study_uid](DCMMKDIR_CONTEXT &dc) { return 0 == strcmp(dot_or_study_uid, dc.dot_or_study_uid); }))
        NamedPipe_CreateClientProc(dot_or_study_uid);
    
    return itlist;
}

static char pipeName[128] = "";
static HANDLE hPipeEvent = NULL, hPipe = NULL;
static OVERLAPPED olPipeConnectOnly;

void NamedPipe_CloseHandle(bool close_event)
{
    if(hPipe && hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    hPipe = NULL;
    if(close_event && hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipeEvent);
        hPipeEvent = NULL;
    }
}

DWORD NamedPipe_CreateListening(const char *pipe_name, bool wait)
{   // manual reset, init signaled, unnamed event object
    DWORD gle = 0;
    if(!hPipeEvent) hPipeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (hPipeEvent == NULL)
    {
        return displayErrorToCerr(__FUNCSIG__ " CreateEvent()", GetLastError());
    }
    memset(&olPipeConnectOnly, 0, sizeof(OVERLAPPED));
    olPipeConnectOnly.hEvent = hPipeEvent;
    if(strlen(pipeName) == 0) sprintf_s(pipeName, "\\\\.\\pipe\\%s", pipe_name ? pipe_name : sessionId);
    hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | (wait ? 0 : FILE_FLAG_OVERLAPPED),
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-type pipe, message read mode, blocking mode
        PIPE_UNLIMITED_INSTANCES, FILE_ASYN_BUF_SIZE, FILE_ASYN_BUF_SIZE, // output, input buffer size 
        0, NULL); // time-out for client run WaitNamedPipe(NMPWAIT_USE_DEFAULT_WAIT), 0 means default(50 ms)
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        return displayErrorToCerr(__FUNCSIG__ " CreateNamedPipe()", GetLastError());
    }

    BOOL fConnected = ConnectNamedPipe(hPipe, (wait ? NULL : &olPipeConnectOnly));
    gle = GetLastError();
    if(wait)  // synchronous
    {
        if(fConnected)
        {
            if(SetEvent(olPipeConnectOnly.hEvent))
                return ERROR_PIPE_CONNECTED;
            else
            {
                gle = GetLastError();
                displayErrorToCerr(__FUNCSIG__ " connect synchronous", gle);
                NamedPipe_CloseHandle();
                return gle;
            }
        }
        else
        {
            displayErrorToCerr(__FUNCSIG__ " connect synchronous", gle);
            NamedPipe_CloseHandle();
            return gle;
        }
    }
    else if(fConnected)  // asynchronous faild
    {
        displayErrorToCerr(__FUNCSIG__ " connect synchronous", gle);
        NamedPipe_CloseHandle();
        return gle;
    }
    // else asynchronous succeed

    switch(gle)
    {
    case ERROR_IO_PENDING:
        break;
    case ERROR_PIPE_CONNECTED:
        if(!SetEvent(olPipeConnectOnly.hEvent))
        {
            gle = GetLastError();
            displayErrorToCerr(__FUNCSIG__ " SetEvent()", gle);
            NamedPipe_CloseHandle();
        }
        break;
    default:
        displayErrorToCerr(__FUNCSIG__ " unexpected error", gle);
        NamedPipe_CloseHandle();
        break;
    }
    return gle;
}

DWORD NamedPipe_CreateClientProc(const char *dot_or_study_uid)
{
#ifdef _DEBUG
    int mkdir_pos = 0;
    char cmd[1024] = __FILE__;
    char *p = strrchr(cmd, '\\');
    if(p)
    {
        ++p;
        strcpy_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmmkdir.exe --general-purpose-dvd -A ");
        mkdir_pos = strlen(cmd);
    }
    else
        mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", pacs_base);
#else
    char cmd[1024];
	int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", pacs_base);
#endif
    if(dot_or_study_uid == NULL || strlen(dot_or_study_uid) == 0)
    {
        cerr << __FUNCSIG__ " : Study UID must be not NULL" << endl;
        return -1;
    }
    if(strcmp(".", dot_or_study_uid))
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s +id %s +D %s.dir --viewer GE -pn %s #", opt_verbose ? "-v" : "", dot_or_study_uid, dot_or_study_uid, sessionId);
    else
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s +D DICOMDIR --viewer GE -pn %s #", opt_verbose ? "-v" : "", sessionId);

    string logfile("_");
    logfile.append(strcmp(".", dot_or_study_uid) ? dot_or_study_uid : "dicomdir").append(".txt");
    HANDLE log = CreateFile(logfile.c_str(), GENERIC_READ | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_NORMAL, NULL);
    if(log == INVALID_HANDLE_VALUE)
    {
        return displayErrorToCerr( __FUNCSIG__ " CreateFile() ", GetLastError());
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
        DisplayErrorToFileHandle(__FUNCSIG__ " CreateProcess()", gle, log);
        CloseHandle(si.hStdOutput);
        CloseHandle(si.hStdError);
        CloseHandle(log);
        return gle;
    }
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);

    DCMMKDIR_CONTEXT dc;
    memset(&dc, 0, sizeof(DCMMKDIR_CONTEXT));
    dc.hProcess = pi.hProcess;
    dc.hThread = pi.hThread;
    dc.log = log;
    strcpy_s(dc.dot_or_study_uid, dot_or_study_uid);

    list<DCMMKDIR_CONTEXT>::iterator it = find_if(list_dir_workers.begin(), list_dir_workers.end(), 
        [dot_or_study_uid](DCMMKDIR_CONTEXT &dc) { return 0 == strcmp(dot_or_study_uid, dc.dot_or_study_uid); });
    if(it == list_dir_workers.end())
        list_dir_workers.push_back(dc);
    else
        *it = dc;
    return 0;
}

static void CALLBACK NamedPipe_ReadPipeComplete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
static void CALLBACK NamedPipe_WritePipeComplete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);

static void push_cmove_log_context_to_dcmmkdir_queue(const CMOVE_LOG_CONTEXT &over_lc)
{
    // push instance to dicomdir queue, "." is dicomdir for all study.
    create_or_open_dicomdir_queue(".")->second.push_back(over_lc);
    list<LPPIPEINST>::iterator it = find_if(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(), 
            [](LPPIPEINST ppi) { return 0 == strcmp(ppi->dot_or_study_uid, "."); });
    // if the queue in block mode, shall call NamedPipe_ReadPipeComplete(.|restart) to active queue.
    // otherwise, don't call NamedPipe_ReadPipeComplete()! it may send 2 files concurrently!
    if(it != list_blocked_pipe_instances.end())
    {
        const char read_message[] = ".|restart";
        LPPIPEINST ppi = *it;
        list_blocked_pipe_instances.erase(it);
        strcpy_s(ppi->chBuffer, read_message);
        ppi->oOverlap.InternalHigh = sizeof(read_message) - 1;
        ppi->cbShouldWrite = ppi->oOverlap.InternalHigh;
        ppi->oOverlap.Internal = 0; // error code
            
        NamedPipe_ReadPipeComplete(0, ppi->oOverlap.InternalHigh, (LPOVERLAPPED)ppi);
    }

    if(strlen(over_lc.file.studyUID) > 0)
    {
        create_or_open_dicomdir_queue(over_lc.file.studyUID)->second.push_back(over_lc);
        it = find_if(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(), 
            [&over_lc](LPPIPEINST ppi) { return 0 == strcmp(ppi->dot_or_study_uid, over_lc.file.studyUID); });
        // same as queue "."
        if(it != list_blocked_pipe_instances.end())
        {
            LPPIPEINST ppi = *it;
            list_blocked_pipe_instances.erase(it);
            ppi->oOverlap.InternalHigh = sprintf_s(ppi->chBuffer, "%s|restart", over_lc.file.studyUID);
            ppi->cbShouldWrite = ppi->oOverlap.InternalHigh;
            ppi->oOverlap.Internal = 0; // error code
            
            NamedPipe_ReadPipeComplete(0, ppi->oOverlap.InternalHigh, (LPOVERLAPPED)ppi);
        }
    }
    else
        cerr << __FUNCSIG__ " : CMOVE_LOG_CONTEXT.file.studyUID is empty." << endl;
}

int compress_queue_to_workers(CMOVE_LOG_CONTEXT *plc)
{
    if(plc)
    {
        queue_compress.push_back(*plc);
        cout << "trigger que_compr "  << plc->file.filename << endl;
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
        displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError());
    CloseHandle(lpPipeInst->hPipeInst);
    if (lpPipeInst != NULL) delete lpPipeInst;
}

static bool check_reading_message(LPPIPEINST lpPipeInst, DWORD cbBytesRead, string &studyUID, string &filename, string &xfer, bool confirm = true)
{
    char *sp1 = strchr(lpPipeInst->chBuffer, '|');
    if(sp1 == NULL) return false;
    studyUID.append(lpPipeInst->chBuffer, sp1 - lpPipeInst->chBuffer);
    ++sp1;
    char *sp2 = strchr(sp1, '|');
    if(sp2 == NULL)
    {
        size_t otherLen = strlen(sp1);
        if(otherLen > 0) filename.append(sp1, otherLen);
    }
    else
    {
        filename.append(sp1, sp2 - sp1);
        ++sp2;
        xfer.append(sp2);
    }
    return !(confirm && studyUID.compare(lpPipeInst->dot_or_study_uid));
}

bool ready_to_close_dcmmkdir_workers = false;
bool is_idle(const char *studyUID)
{
    if(hDirNotify == NULL && 0 == workers.size() && 0 == queue_compress.size())
    {
        size_t dir_que_size = 0;

        if(studyUID)
            dir_que_size = map_dir_queue_list[studyUID].size();
        else
            dir_que_size = accumulate(map_dir_queue_list.begin(), map_dir_queue_list.end(), 0,
                [](size_t accumulator, pair<string, list<CMOVE_LOG_CONTEXT> > p) {
                    return accumulator + p.second.size(); });

        if(dir_que_size > 0) return false;

        if(studyUID)
            return (list_blocked_pipe_instances.end() != 
                find_if(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(),
                    [studyUID](LPPIPEINST ppi) { return 0 == strcmp(studyUID, ppi->dot_or_study_uid); }));
        else
            return (0 >= (list_dir_workers.size() - list_blocked_pipe_instances.size()));
    }
    else
        return false;
}

void close_all_blocked_pipe_instances()
{
    for(list<DCMMKDIR_CONTEXT>::iterator it_dc = list_dir_workers.begin(); it_dc != list_dir_workers.end(); ++it_dc)
    {
        if(it_dc->detachPipeInstance) continue;

        DCMMKDIR_CONTEXT &dc = *it_dc;
        list<LPPIPEINST>::iterator it_ppi = find_if(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(),
            [&dc](LPPIPEINST ppi) { return 0 == strcmp(dc.dot_or_study_uid, ppi->dot_or_study_uid); });
        if(it_ppi == list_blocked_pipe_instances.end())
        {
            cerr << __FUNCSIG__ " : not matching pipe instance " << dc.dot_or_study_uid << endl;
        }
        else
        {
            DisconnectAndClose(*it_ppi);
            list_blocked_pipe_instances.erase(it_ppi);
            it_dc->detachPipeInstance = true;
        }
    }
    if(!list_blocked_pipe_instances.empty())
    {
        for_each(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(),
            [](LPPIPEINST ppi) {
                cerr << __FUNCSIG__ " : standalone pipe instance " << ppi->dot_or_study_uid << endl;
            });
    }
}

static void CALLBACK NamedPipe_ReadPipeComplete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;
    if(dwErr == 0)
    {
        if(cbBytesRead == 0)
            cerr << __FUNCSIG__ " : cbBytesRead is 0" << endl;
        else
            lpPipeInst->chBuffer[cbBytesRead] = '\0';

        string studyUID, filename, xfer;
        // extract studyUID and filename from message
        if(!check_reading_message(lpPipeInst, cbBytesRead, studyUID, filename, xfer))
        {
            cerr << __FUNCSIG__ " check_reading_message(): message is corrupt, " << lpPipeInst->chBuffer << endl;
            DisconnectAndClose(lpPipeInst);
            return;
        }

        // find study's queue from map by studyUID
        map<string, list<CMOVE_LOG_CONTEXT> >::iterator it = create_or_open_dicomdir_queue(studyUID.c_str());
        if(it == map_dir_queue_list.end())
        {
            cerr << __FUNCSIG__ " create_or_open_dicomdir_queue(): Study UID is corrupt, " << lpPipeInst->chBuffer << endl;
            DisconnectAndClose(lpPipeInst);
            return;
        }
        list<CMOVE_LOG_CONTEXT> &queue_clc = it->second;  // get queue

        list<CMOVE_LOG_CONTEXT>::iterator it_clc = queue_clc.end();
        // is the message send by dcmmkdir?
        // restart shall only send by server's call: push_cmove_log_context_to_dcmmkdir_queue().
        // restart: some file is ready in queue, send it to dcmmkdir, queue shall leave block mode.
        // dcmmkdi: dcmmkdir pid xxxx, dcmmkdir first bind.
        if(strncmp(filename.c_str(), "restart", 7) && strncmp(filename.c_str(), "dcmmkdir", 7))
        {   // yes, real message
            it_clc = find_if(queue_clc.begin(), queue_clc.end(),
                [&filename](CMOVE_LOG_CONTEXT &lc){ return filename.compare(lc.file.unique_filename) == 0; });
            if(it_clc != queue_clc.end())
            {
                cout << "trigger make_dicomdir " << studyUID << "\\" << it_clc->file.filename << endl;
                if(studyUID != ".")
                {
                    strcpy_s(it_clc->file.xfer_new, xfer.c_str());
                    make_index(*it_clc);

                    // send notification of a file OK to state dir
                    stringstream output;
                    output << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPRESS_OK << endl;
                    output << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << it_clc->file.tag
                        << " " << it_clc->file.filename << " " << it_clc->file.unique_filename << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00100020 ";
                    x_www_form_codec_encode(it_clc->file.patientID, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 0020000D " << it_clc->file.studyUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 0020000E " << it_clc->file.seriesUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00080018 " << it_clc->file.instanceUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00020010 " << it_clc->file.xfer << " " << it_clc->file.isEncapsulated << " " << it_clc->file.xfer_new << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100010 ";
                    x_www_form_codec_encode(it_clc->patient.patientsName, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100030 " << it_clc->patient.birthday << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100040 " << it_clc->patient.sex << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00101020 " << it_clc->patient.height << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00101030 " << it_clc->patient.weight << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080020 " << it_clc->study.studyDate << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080030 " << it_clc->study.studyTime << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080050 ";
                    x_www_form_codec_encode(it_clc->study.accessionNumber, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_SERIES << " 00080060 " << it_clc->series.modality << endl;
                    string notify = output.str();
                    output.str("");

                    char notify_file_name[MAX_PATH];
                    size_t pos = in_process_sequence(notify_file_name, sizeof(notify_file_name), STATE_DIR);
                    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
                    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out);
                    if(ntf.good())
                    {
                        ntf << notify ;
                        ntf.close();
                    }
                    else
                        cerr << notify;
                }
                // this instance is all OK, erase it from queue
                queue_clc.erase(it_clc);
            }
            else
                cerr << __FUNCSIG__ " NamePipe read file's name is not in queue: " << lpPipeInst->chBuffer << endl;
        }

        if(queue_clc.empty())
        {
            // no more file in queue, add to lpPipeInst list_blocked_pipe_instances.
            // queue has blocked until "studyUID|restart" message arriving.
            list<LPPIPEINST>::iterator it = find_if(list_blocked_pipe_instances.begin(), list_blocked_pipe_instances.end(), 
                [lpPipeInst](LPPIPEINST ppi) { return 0 == strcmp(ppi->dot_or_study_uid, lpPipeInst->dot_or_study_uid); });
            if(it == list_blocked_pipe_instances.end())
                list_blocked_pipe_instances.push_back(lpPipeInst);
        }
        else
        {
            it_clc = queue_clc.begin();
            lpPipeInst->cbShouldWrite = sprintf_s(lpPipeInst->chBuffer, "%s|%s", it_clc->file.studyUID, it_clc->file.unique_filename);
            //replace(lpPipeInst->chBuffer, lpPipeInst->chBuffer + strlen(lpPipeInst->chBuffer), '/', '\\');

            if(!WriteFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer, lpPipeInst->cbShouldWrite, 
                (LPOVERLAPPED) lpPipeInst, (LPOVERLAPPED_COMPLETION_ROUTINE) NamedPipe_WritePipeComplete))
            {
                DWORD gle = GetLastError();
                if(gle != ERROR_INVALID_USER_BUFFER && gle != ERROR_NOT_ENOUGH_MEMORY)
                {
                    displayErrorToCerr(__FUNCSIG__ " WriteFileEx()", gle);
                    DisconnectAndClose(lpPipeInst);
                }
            }
        }
    }
    else // dwErr != 0
    {
        displayErrorToCerr(__FUNCSIG__ " param dwErr is an error", dwErr);
        DisconnectAndClose(lpPipeInst);
    }
}

static void CALLBACK NamedPipe_WritePipeComplete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST) lpOverLap;
    BOOL fRead = FALSE;
    // The read operation has finished, so write a response (if no error occurred).
    if ((dwErr == 0) && (cbBytesWrite == lpPipeInst->cbShouldWrite))
    {
        // next read loop, until dcmmkdir close pipe
        fRead = ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer,
            FILE_ASYN_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)NamedPipe_ReadPipeComplete);
    }
    // Disconnect if an error occurred.
    if (! fRead) DisconnectAndClose(lpPipeInst);
}

static void CALLBACK NamedPipe_FirstReadBindPipe(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;

    BOOL bindOK = FALSE;
    // if first bind failed, how to?
    if ((dwErr == 0) && (cbBytesRead != 0))
    {
        string studyUID, otherMessage, xfer;
        // don't confirm studyUID == lpPipeInst->dot_or_study_uid, it's unbound.
        if(!check_reading_message(lpPipeInst, cbBytesRead, studyUID, otherMessage, xfer, false))
        {
            cerr << __FUNCSIG__ " : message is currupt, " << studyUID << "|" << otherMessage << endl;
            DisconnectAndClose(lpPipeInst);
            return;
        }
        list<DCMMKDIR_CONTEXT>::iterator itDirWorker = find_if(list_dir_workers.begin(), list_dir_workers.end(), 
            [&studyUID](DCMMKDIR_CONTEXT &dc) { return 0 == strcmp(studyUID.c_str(), dc.dot_or_study_uid); });
        if(itDirWorker == list_dir_workers.end())
        {
            cerr << __FUNCSIG__ " : can't bind incoming request from client process, " << studyUID << "|" << otherMessage << endl;
            DisconnectAndClose(lpPipeInst);
            return;
        }
        strcpy_s(lpPipeInst->dot_or_study_uid, itDirWorker->dot_or_study_uid);
        NamedPipe_ReadPipeComplete(dwErr, cbBytesRead, lpOverLap);
    }
    else
    {
        displayErrorToCerr(__FUNCSIG__, dwErr);
        DisconnectAndClose(lpPipeInst);
    }
}

static DWORD NamedPipe_PipeConnected(HANDLE)
{
    DWORD gle = 0, cbRet = 0;
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, FALSE))
    {
        LPPIPEINST lpPipeInst = new PIPEINST;
        memset(lpPipeInst, 0, sizeof(PIPEINST));
        lpPipeInst->hPipeInst = hPipe;
        if(! ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer,
                FILE_ASYN_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
                (LPOVERLAPPED_COMPLETION_ROUTINE)NamedPipe_FirstReadBindPipe))
        {
            gle = GetLastError();
            displayErrorToCerr(__FUNCSIG__ " ReadFileEx()", gle);
            delete lpPipeInst;
        }
    }
    else
    {
        gle = GetLastError();
        displayErrorToCerr(__FUNCSIG__ " GetOverlappedResult()", gle);
    }
    if(gle)
        NamedPipe_CloseHandle();

    // start a listening named pipe
    gle = NamedPipe_CreateListening(sessionId, false);
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        displayErrorToCerr(__FUNCSIG__ " NamedPipe_CreateListening()", gle);
        NamedPipe_CloseHandle();
        return gle;
    }
    return ERROR_SUCCESS;
}

DWORD worker_complete(DWORD wr, HANDLE *objs, WORKER_CALLBACK* cbs, size_t worker_num)
{
    CMOVE_LOG_CONTEXT over_lc;
    wr -= WAIT_OBJECT_0;

    // not compress worker
    if(cbs && cbs[wr]) return cbs[wr](objs[wr]);

    // compress worker, find worker's CMOVE_LOG_CONTEXT
    HANDLE over_hProcess = objs[wr];
    list<CMOVE_LOG_CONTEXT>::iterator it = find_if(workers.begin(), workers.end(), 
        [over_hProcess](CMOVE_LOG_CONTEXT &clc) { return clc.hprocess == over_hProcess; });
    over_lc = *it;
    workers.erase(it);
    cout << "trigger complete " << over_lc.file.filename << endl;

    bool result = false;
    if(over_lc.hprocess != INVALID_HANDLE_VALUE)
    {
        CloseHandle(over_lc.hthread);
        over_lc.hthread = INVALID_HANDLE_VALUE;
        CloseHandle(over_lc.hprocess);
        over_lc.hprocess = INVALID_HANDLE_VALUE;
        close_log(over_lc.log);
        over_lc.log = INVALID_HANDLE_VALUE;

        push_cmove_log_context_to_dcmmkdir_queue(over_lc);
    }
    compress_queue_to_workers(NULL);
    return 0;
}

HANDLE *get_worker_handles(size_t *worker_num, size_t *queue_size, WORKER_CALLBACK ** ppCBs, size_t reserve)
{
    if(queue_size) *queue_size = queue_compress.size() +
        accumulate(map_dir_queue_list.begin(), map_dir_queue_list.end(), 0,
            [](size_t accumulator, pair<string, list<CMOVE_LOG_CONTEXT> > p) {
                return accumulator + p.second.size(); });
    size_t wk_num = workers.size();
    
    bool hasPipeEvent = (hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE);
    if(hasPipeEvent) ++wk_num;
    
    wk_num += list_dir_workers.size();

    if(reserve) wk_num += reserve;

    if(wk_num > 0)
    {
        HANDLE *objs = new HANDLE[wk_num];
        memset(objs, 0, sizeof(HANDLE) * wk_num);
        if(ppCBs) *ppCBs = new WORKER_CALLBACK[wk_num];
        memset(*ppCBs, 0, sizeof(WORKER_CALLBACK) * wk_num);
        size_t i = reserve;
        if(hasPipeEvent)
        {
            objs[i] = hPipeEvent;
            (*ppCBs)[i++] = NamedPipe_PipeConnected;
        }
        if(workers.size())
            transform(workers.begin(), workers.end(), objs + i, 
                [](const CMOVE_LOG_CONTEXT &clc) { return clc.hprocess; });
        i += workers.size();

        if(list_dir_workers.size())
            transform(list_dir_workers.begin(), list_dir_workers.end(), objs + i, 
                [](const DCMMKDIR_CONTEXT &dc) { return dc.hProcess; });

        for(size_t dir_num = 0; dir_num < list_dir_workers.size(); ++dir_num)
            (*ppCBs)[i + dir_num] = close_dcmmkdir_worker;

        if(worker_num) *worker_num = wk_num;
        return objs;
    }
    else
    {
        if(worker_num) *worker_num = 0;
        if(queue_size) *queue_size = 0;
        if(ppCBs) *ppCBs = NULL;
        return NULL;
    }
}
