#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;
using namespace handle_context;

named_pipe_server* named_pipe_server::singleton_ptr = NULL;

void CALLBACK named_pipe_server::client_connect_callback_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{ named_pipe_server::singleton_ptr->client_connect_callback(dwErr, cbBytesRead, lpOverLap); }
void CALLBACK named_pipe_server::write_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{ named_pipe_server::singleton_ptr->write_pipe_complete(dwErr, cbBytesWrite, lpOverLap); }
void CALLBACK named_pipe_server::read_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{ named_pipe_server::singleton_ptr->read_pipe_complete(dwErr, cbBytesRead, lpOverLap); }
void named_pipe_server::register_named_pipe_server(named_pipe_server *p)
{ singleton_ptr = p; }

named_pipe_server::named_pipe_server(const char *pipe_path, std::ostream *plog)
    : handle_waitable(pipe_path, plog), hPipeEvent(NULL), hPipe(NULL)
{
    memset(&olPipeConnectOnly, 0, sizeof(OVERLAPPED));
}

named_pipe_server& named_pipe_server::operator=(const named_pipe_server &r)
{
    handle_waitable::operator=(r);
    pflog = r.pflog;
    hPipeEvent = r.hPipeEvent;
    hPipe = r.hPipe;
    olPipeConnectOnly = r.olPipeConnectOnly;
    return *this;
}

DWORD named_pipe_server::start_listening()
{
    hPipe = INVALID_HANDLE_VALUE;
    if(!hPipeEvent) hPipeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if(hPipeEvent == NULL)
        return displayErrorToCerr(__FUNCSIG__ " CreateEvent()", GetLastError(), pflog);
    olPipeConnectOnly.hEvent = hPipeEvent;

    string pipe_path("\\\\.\\pipe\\");
    pipe_path.append(get_path());
    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" listen on " << pipe_path << endl;
    hPipe = CreateNamedPipe(pipe_path.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-type pipe, message read mode, blocking mode
        PIPE_UNLIMITED_INSTANCES, FILE_BUF_SIZE, FILE_BUF_SIZE, // output, input buffer size 
        0, NULL); // time-out for client run WaitNamedPipe(NMPWAIT_USE_DEFAULT_WAIT), 0 means default(50 ms)
    if(hPipe == INVALID_HANDLE_VALUE)
        return displayErrorToCerr(__FUNCSIG__ " CreateNamedPipe()", GetLastError(), pflog);

    DWORD gle = 0;
    BOOL fConnected = ConnectNamedPipe(hPipe, &olPipeConnectOnly);
    gle = GetLastError();
    if(fConnected)  // asynchronous faild
    {
        return displayErrorToCerr(__FUNCSIG__ " connect synchronous", gle, pflog);
    }

    switch(gle)
    {
    case ERROR_IO_PENDING:
        break;
    case ERROR_PIPE_CONNECTED:
        if(!SetEvent(olPipeConnectOnly.hEvent))
        {
            gle = GetLastError();
            displayErrorToCerr(__FUNCSIG__ " SetEvent()", gle, pflog);
        }
        break;
    default:
        displayErrorToCerr(__FUNCSIG__ " unexpected error", gle, pflog);
        break;
    }
    return gle;
}

// first message is: study_uid|dcmmkdir pid <pid>
bool named_pipe_server::check_reading_message(LPPIPEINST lpPipeInst, DWORD cbBytesRead, string &study_uid, string &filename, string &xfer, bool confirm_study_uid)
{
    char *sp1 = strchr(lpPipeInst->chBuffer, '|');
    if(sp1 == NULL) return false;
    study_uid.append(lpPipeInst->chBuffer, sp1 - lpPipeInst->chBuffer);
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
    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" check message: " << study_uid << "|" << filename << endl;
    return !(confirm_study_uid && study_uid.compare(lpPipeInst->study_uid));
}

void named_pipe_server::disconnect_connection_auto_detect(LPPIPEINST lpPipeInst)
{
    if(lpPipeInst == NULL) return;
    string study_uid(lpPipeInst->study_uid);
    STUDY_MAP::iterator it = map_study.find(study_uid);
    if(it == map_study.end())
    {
        if(lpPipeInst->hPipeInst && lpPipeInst->hPipeInst != INVALID_HANDLE_VALUE)
        {
            DisconnectNamedPipe(lpPipeInst->hPipeInst);
            CloseHandle(lpPipeInst->hPipeInst);
            lpPipeInst->hPipeInst = NULL;
        }
    }
    else
    {
        handle_study* phs = map_study[study_uid];
        map_study.erase(study_uid);
        if(phs) delete phs;
    }
}

void named_pipe_server::client_connect_callback(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = reinterpret_cast<LPPIPEINST>(lpOverLap);

    BOOL bindOK = FALSE;
    if ((dwErr == 0) && (cbBytesRead != 0))
    {
        string study_uid, otherMessage, xfer;
        // first message is: study_uid|dcmmkdir pid <pid>
        // false param: don't confirm studyUID == lpPipeInst->study_uid, because it's unbound.
        if(!check_reading_message(lpPipeInst, cbBytesRead, study_uid, otherMessage, xfer, false))
        {
            time_header_out(*pflog) << __FUNCSIG__ " message is currupt, " << study_uid << "|" << otherMessage << endl;
            if(lpPipeInst->hPipeInst && lpPipeInst->hPipeInst != INVALID_HANDLE_VALUE)
            {
                DisconnectNamedPipe(lpPipeInst->hPipeInst);
                CloseHandle(lpPipeInst->hPipeInst);
            }
            delete lpPipeInst;
            return;
        }
        strcpy_s(lpPipeInst->study_uid, study_uid.c_str());

        handle_study *phs = make_handle_study(study_uid);
        if(phs)
        {   // bind named pipe handle and handle_study
            phs->bind_pipe_context(lpPipeInst);
            DWORD gle = phs->write_message_to_pipe();
            if(gle)
            {
                time_header_out(*pflog) << __FUNCSIG__ " write_message_to_pipe() failed, deconstruct handle_study " << study_uid << endl;
                disconnect_connection_auto_detect(lpPipeInst);
            }
            else // OK
                if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" client connect OK, client write: " << study_uid << "|" << otherMessage << endl;
        }
        else
        {
            time_header_out(*pflog) << __FUNCSIG__ " can't find handle_study(" << study_uid << "), otherMessage " << otherMessage << endl;
            delete lpPipeInst;
        }
    }
    else
    {
        if(dwErr) displayErrorToCerr(__FUNCSIG__, dwErr, pflog);
        else time_header_out(*pflog) << __FUNCSIG__" failed: read " << lpPipeInst->chBuffer << endl;
        if(lpPipeInst)
        {
            if(lpPipeInst->hPipeInst && lpPipeInst->hPipeInst != INVALID_HANDLE_VALUE)
            {
                DisconnectNamedPipe(lpPipeInst->hPipeInst);
                CloseHandle(lpPipeInst->hPipeInst);
            }
            delete lpPipeInst;
        }
    }
}

void named_pipe_server::write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST) lpOverLap;
    DWORD gle = 0;
    // The read operation has finished, so write a response (if no error occurred).
    if ((dwErr == 0) && (cbBytesWrite == lpPipeInst->cbShouldWrite))
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" " << lpPipeInst->chBuffer << endl;
        // next read loop, until dcmmkdir close pipe
        if(!ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer,
            FILE_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)read_pipe_complete_func_ptr))
            gle = GetLastError();
    }
    else
    {
        if(dwErr) displayErrorToCerr(__FUNCSIG__, dwErr, pflog);
        else time_header_out(*pflog) << __FUNCSIG__" failed: message written " << lpPipeInst->chBuffer << endl;
    }
    // Disconnect if an error occurred.
    if (gle)
    {
        displayErrorToCerr(__FUNCSIG__" ReadFileEx()", gle, pflog);
        disconnect_connection_auto_detect(lpPipeInst);
    }
}

void named_pipe_server::read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST)lpOverLap;
    if(dwErr == 0)
    {
        if(cbBytesRead == 0)
            time_header_out(*pflog) << __FUNCSIG__ " : cbBytesRead is 0" << endl;
        else
            lpPipeInst->chBuffer[cbBytesRead] = '\0';

        string study_uid, filename, xfer;
        // extract studyUID and filename from message
        if(!check_reading_message(lpPipeInst, cbBytesRead, study_uid, filename, xfer))
        {
            time_header_out(*pflog) << __FUNCSIG__ " check_reading_message(): message is corrupt, " << lpPipeInst->chBuffer << endl;
            disconnect_connection_auto_detect(lpPipeInst);
            return;
        }

        // find handle_study by study_uid
        STUDY_MAP::iterator it = map_study.find(study_uid);
        if(it == map_study.end())
        {
            time_header_out(*pflog) << __FUNCSIG__ " pipe message read is corrupt, " << lpPipeInst->chBuffer << endl;
            disconnect_connection_auto_detect(lpPipeInst);
            return;
        }
        handle_study *phs = it->second;

        if(phs) // handle_study is found
        {
            // previous message is compress ok message, make xml index, erease the action from list
            phs->action_compress_ok(filename, xfer);
            if(phs->write_message_to_pipe()) disconnect_connection_auto_detect(lpPipeInst);
        }
        else
        {
            time_header_out(*pflog) << __FUNCSIG__ " can't find study_uid " << study_uid << endl;
            disconnect_connection_auto_detect(lpPipeInst);
        }
    }
    else // dwErr != 0
    {
        displayErrorToCerr(__FUNCSIG__ " param dwErr is an error", dwErr, pflog);
        disconnect_connection_auto_detect(lpPipeInst);
    }
}

DWORD named_pipe_server::pipe_client_connect_incoming()
{
    DWORD gle = 0, cbRet = 0;
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, FALSE))
    {
        LPPIPEINST lpPipeInst = new PIPEINST;
        memset(lpPipeInst, 0, sizeof(PIPEINST));
        lpPipeInst->hPipeInst = hPipe; // bind hPipe to named pipe instance
        if(! ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer, FILE_BUF_SIZE, 
            (LPOVERLAPPED)lpPipeInst, (LPOVERLAPPED_COMPLETION_ROUTINE)client_connect_callback_func_ptr))
        {
            gle = displayErrorToCerr(__FUNCSIG__ " ReadFileEx()", GetLastError(), pflog);
            delete lpPipeInst;
        }
    }
    else gle = displayErrorToCerr(__FUNCSIG__ " GetOverlappedResult()", GetLastError(), pflog);

    if(gle) DisconnectNamedPipe(hPipe);
    // discard this connected named pipe instance
    // start a new listening named pipe, hPipe is changed
    gle = start_listening();
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
    {
        if(hPipe == INVALID_HANDLE_VALUE)
        {
            DisconnectNamedPipe(hPipe);
            hPipe = NULL;
        }
        return displayErrorToCerr(__FUNCSIG__ " start_listening()", gle, pflog);
    }
    return 0; //ERROR_SUCCESS
}

handle_study* named_pipe_server::make_handle_study(const std::string &study_uid)
{
    handle_study *phs = map_study[study_uid];
    if(phs == NULL)
    {
        time_header_out(*pflog) << "watch_notify() handle_compress complete, can't find map_study[" << study_uid << "], create new handle_study" << endl;
        char dicomdir[1024], hash[9];
        HashStr(study_uid.c_str(), hash, sizeof(hash));
        int pos = sprintf_s(dicomdir, "%s\\pacs\\archdir\\v0000000\\%c%c\\%c%c\\%c%c\\%c%c",
            GetPacsBase(), hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7]);
        MkdirRecursive(dicomdir);

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
            mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", GetPacsBase());
#else
        char cmd[1024];
	    int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd -A ", GetPacsBase());
#endif
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s +id %s +D %s.dir --viewer GE -pn %s #", 
            opt_verbose ? "-v" : "", study_uid.c_str(), study_uid.c_str(), get_path().c_str());

        phs = new handle_study(dicomdir, cmd, "dcmmkdir", dicomdir, study_uid, pflog);
        if(phs)
        {
            if(0 == phs->start_process(*pflog)) map_study[study_uid] = phs;
            else { delete phs; phs = NULL; }
        }
        else time_header_out(*pflog) << __FUNCSIG__" new handle_study() failed." << endl;
    }
    return phs;
}

named_pipe_server::~named_pipe_server()
{
    if(hPipe && hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    hPipe = NULL;
    if(hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipeEvent);
        hPipeEvent = NULL;
    }
    if(opt_verbose) time_header_out(*pflog) << "named_pipe_server::~named_pipe_server()" << endl;
    ostream *pflog2 = pflog;
    for_each(map_study.begin(), map_study.end(), [pflog2](const STUDY_PAIR &p) { p.second->print_state(*pflog2); delete p.second; });
}