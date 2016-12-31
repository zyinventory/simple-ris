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
    : base_path(pipe_path, plog), hPipeEvent(NULL), hPipe(NULL)
{
    memset(&olPipeConnectOnly, 0, sizeof(OVERLAPPED));
}

named_pipe_server& named_pipe_server::operator=(const named_pipe_server &r)
{
    base_path::operator=(r);
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
    hPipe = CreateNamedPipe(pipe_path.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-type pipe, message read mode, blocking mode
        PIPE_UNLIMITED_INSTANCES, FILE_BUF_SIZE, FILE_BUF_SIZE, // output, input buffer size 
        0, NULL); // time-out for client run WaitNamedPipe(NMPWAIT_USE_DEFAULT_WAIT), 0 means default(50 ms)
    if(hPipe == INVALID_HANDLE_VALUE)
        return displayErrorToCerr(__FUNCSIG__ " CreateNamedPipe()", GetLastError(), pflog);
    else if(opt_verbose)
        time_header_out(*pflog) << __FUNCSIG__" listen on " << hPipe << ": " << pipe_path << endl;

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
    if(0 == map_study.count(study_uid))
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
        if(phs) delete phs; // handle_study will delete lpPipeInst, don't delete twice
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

        handle_study *phs = find_handle_study(study_uid);
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
                if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" client connect OK." << endl;
        }
        else
        {
            time_header_out(*pflog) << __FUNCSIG__ " can't find handle_study(" << study_uid << "), otherMessage " << otherMessage << endl;
			disconnect_connection_auto_detect(lpPipeInst);
        }
    }
    else
    {
        if(dwErr) displayErrorToCerr(__FUNCSIG__, dwErr, pflog);
        else time_header_out(*pflog) << __FUNCSIG__" failed: read " << lpPipeInst->chBuffer << endl;
		disconnect_connection_auto_detect(lpPipeInst);
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
            phs->remove_compress_ok_action(filename, xfer);
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
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, TRUE))
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" async operation complete" << endl;
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
        return displayErrorToCerr(__FUNCSIG__ " start_listening()", gle, pflog);
    return 0; //ERROR_SUCCESS
}

handle_study* named_pipe_server::make_handle_study(const std::string &study_uid)
{
    handle_study *phs = map_study[study_uid];
    if(phs == NULL)
    {
        time_header_out(*pflog) << "create new handle_study " << study_uid << endl;
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
            strcpy_s(p, sizeof(cmd) - (p - cmd), "..\\Debug\\dcmmkdir.exe --general-purpose-dvd +I +A +X -Xs 64 ");
            mkdir_pos = strlen(cmd);
        }
        else
            mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd +I +A +X -Xs 64 ", GetPacsBase());
#else
        char cmd[1024];
	    int mkdir_pos = sprintf_s(cmd, "%s\\bin\\dcmmkdir.exe --general-purpose-dvd +I +A +X -Xs 64 ", GetPacsBase());
#endif
        sprintf_s(cmd + mkdir_pos, sizeof(cmd) - mkdir_pos, "%s +id %s +D %s.dir -pn %s #", 
            opt_verbose ? "-v" : "", study_uid.c_str(), study_uid.c_str(), get_path().c_str());

        phs = new handle_study(dicomdir, cmd, "dcmmkdir", dicomdir, study_uid, pflog);
        if(phs)
        {
            if(0 == phs->start_process(true)) map_study[study_uid] = phs;
            else
            {
                map_study.erase(study_uid);
                delete phs;
                phs = NULL;
            }
        }
        else
        {
            map_study.erase(study_uid);
            time_header_out(*pflog) << __FUNCSIG__" new handle_study() failed." << endl;
        }
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
    for_each(map_study.begin(), map_study.end(), [](const STUDY_PAIR &p) {
        if(p.second)
        {
            if(debug_mode) p.second->print_state();
            delete p.second;
        }
    });
}

void named_pipe_server::check_study_timeout_to_generate_jdf(const std::set<std::string> &queued_study_uids, const std::set<std::string> &exist_assoc_paths)
{
    STUDY_MAP::iterator it = map_study.begin();
    while(it != map_study.end())
    {
        handle_study *phs = it->second;
        if(phs && !phs->send_remain_message_to_pipe() && phs->is_time_out()
            && queued_study_uids.find(phs->get_id()) == queued_study_uids.cend()
            && ! any_of(phs->get_set_association_path().begin(), phs->get_set_association_path().end(),
                [&exist_assoc_paths](const string &s) { return (exist_assoc_paths.find(s) != exist_assoc_paths.end()); }))
        {
            if(opt_verbose) time_header_out(*pflog) << "named_pipe_server::check_study_timeout_to_generate_jdf() study timeout: "
                << phs->get_id() << endl;
            bool auto_publish = phs->get_last_association_action().is_auto_publish();
            if(opt_verbose) time_header_out(*pflog) << "named_pipe_server::check_study_timeout_to_generate_jdf() delete study " << phs->get_id() << endl;
            it = map_study.erase(it);
            phs->print_state();
            delete phs;
        }
        else ++it;
    }
}
