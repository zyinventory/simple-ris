#include "stdafx.h"
#include "commonlib.h"
#include "../dcmdata/include/dcmtk/dcmdata/dconotify.h"

using namespace std;
using namespace handle_context;

named_pipe_server* named_pipe_server::named_pipe_server_ptr = NULL;

static void CALLBACK client_connect_callback_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    named_pipe_server::named_pipe_server_ptr->client_connect_callback(dwErr, cbBytesRead, lpOverLap);
}
static void CALLBACK write_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    named_pipe_server::named_pipe_server_ptr->write_pipe_complete(dwErr, cbBytesWrite, lpOverLap);
}
void CALLBACK read_pipe_complete_func_ptr(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    named_pipe_server::named_pipe_server_ptr->read_pipe_complete(dwErr, cbBytesRead, lpOverLap);
}

void named_pipe_server::register_named_pipe_server(named_pipe_server *p)
{ named_pipe_server_ptr = p; }

named_pipe_server::named_pipe_server(const char *pipe_path, std::ostream *plog)
    : handle_waitable(pipe_path), pflog(plog), hPipeEvent(NULL), hPipe(NULL)
{
    if(pflog == NULL) pflog = &std::cerr;
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

    hPipe = CreateNamedPipe(get_path().c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
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
        displayErrorToCerr(__FUNCSIG__ " connect synchronous", gle, pflog);
        return gle;
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
static bool check_reading_message(LPPIPEINST lpPipeInst, DWORD cbBytesRead, string &studyUID, string &filename, string &xfer, bool confirm_study_uid = true)
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
    return !(confirm_study_uid && studyUID.compare(lpPipeInst->study_uid));
}

void named_pipe_server::disconnect_connection(LPPIPEINST lpPipeInst)
{
    if(lpPipeInst == NULL) return;
    handle_study* phs = map_study[lpPipeInst->study_uid];
    if (! DisconnectNamedPipe(lpPipeInst->hPipeInst))
        displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError(), pflog);
    CloseHandle(lpPipeInst->hPipeInst);
    if(phs)
    {
        map_study.erase(lpPipeInst->study_uid);
        delete phs;
    }
    delete lpPipeInst;
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
            time_header_out(*pflog) << __FUNCSIG__ " : message is currupt, " << study_uid << "|" << otherMessage << endl;
            disconnect_connection(lpPipeInst);
            return;
        }
        STUDY_MAP::iterator it = map_study.find(study_uid);
        if(it == map_study.end())
        {   // if first bind failed, close named pipe instance
            time_header_out(*pflog) << __FUNCSIG__ " : can't bind incoming request from client process, " << study_uid << "|" << otherMessage << endl;
            disconnect_connection(lpPipeInst);
            return;
        }
        // bind named pipe handle and handle_study
        strcpy_s(lpPipeInst->study_uid, it->first.c_str());
        read_pipe_complete(dwErr, cbBytesRead, lpOverLap);
    }
    else
    {   // if first bind failed, close named pipe instance
        displayErrorToCerr(__FUNCSIG__, dwErr, pflog);
        disconnect_connection(lpPipeInst);
    }
}

void named_pipe_server::write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    LPPIPEINST lpPipeInst = (LPPIPEINST) lpOverLap;
    BOOL fRead = FALSE;
    // The read operation has finished, so write a response (if no error occurred).
    if ((dwErr == 0) && (cbBytesWrite == lpPipeInst->cbShouldWrite))
    {
        // next read loop, until dcmmkdir close pipe
        fRead = ReadFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer,
            FILE_BUF_SIZE * sizeof(TCHAR), (LPOVERLAPPED) lpPipeInst,
            (LPOVERLAPPED_COMPLETION_ROUTINE)read_pipe_complete_func_ptr);
    }
    // Disconnect if an error occurred.
    if (! fRead) disconnect_connection(lpPipeInst);
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
            disconnect_connection(lpPipeInst);
            return;
        }

        // find handle_study by study_uid
        STUDY_MAP::iterator it = map_study.find(study_uid); // todo: create new handle if not exist?
        if(it == map_study.end())
        {
            time_header_out(*pflog) << __FUNCSIG__ " pipe message read is corrupt, " << lpPipeInst->chBuffer << endl;
            disconnect_connection(lpPipeInst);
            return;
        }
        handle_study *phs = it->second;

        if(phs) // handle_study is found
        {
            // dcmmkdir: dcmmkdir pid xxxx, dcmmkdir first bind.
            // restart: some file is ready in queue, send it to dcmmkdir, queue shall leave block mode.
            if(strncmp(filename.c_str(), "dcmmkdir", 7) && strncmp(filename.c_str(), "restart", 7))
            {
                // previous message is compress ok message, make xml index, erease the action from list
                list<action_from_association>::iterator it_clc = find_if(phs->list_action.begin(), phs->list_action.end(),
                    [&filename](const action_from_association &lc){ return lc.pnfc && filename.compare(lc.pnfc->file.unique_filename) == 0; });
                if(it_clc != phs->list_action.end())
                {
                    cout << "trigger make_dicomdir " << study_uid << "\\" << it_clc->pnfc->file.filename << endl;

                    strcpy_s(it_clc->pnfc->file.xfer_new, xfer.c_str());
                    // todo: make_index(*it_clc);

                    // send notification of a file dicomdir and index OK to state dir
                    stringstream output;
                    output << NOTIFY_ACKN_ITEM << " " << hex << setw(8) << setfill('0') << uppercase << NOTIFY_COMPRESS_OK << endl;
                    output << NOTIFY_FILE_TAG << " " << hex << setw(8) << setfill('0') << uppercase << it_clc->pnfc->file_seq
                        << " " << it_clc->pnfc->file.filename << " " << it_clc->pnfc->file.unique_filename << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00100020 ";
                    x_www_form_codec_encode_to_ostream(it_clc->pnfc->file.patientID, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 0020000D " << it_clc->pnfc->file.studyUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 0020000E " << it_clc->pnfc->file.seriesUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00080018 " << it_clc->pnfc->file.instanceUID << endl;
                    output << NOTIFY_LEVEL_INSTANCE << " 00020010 " << it_clc->pnfc->file.xfer << " " << it_clc->pnfc->file.isEncapsulated << " " << it_clc->pnfc->file.xfer_new << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100010 ";
                    x_www_form_codec_encode_to_ostream(it_clc->pnfc->patient.patientsName, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100030 " << it_clc->pnfc->patient.birthday << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00100040 " << it_clc->pnfc->patient.sex << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00101020 " << it_clc->pnfc->patient.height << endl;
                    output << NOTIFY_LEVEL_PATIENT << " 00101030 " << it_clc->pnfc->patient.weight << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080020 " << it_clc->pnfc->study.studyDate << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080030 " << it_clc->pnfc->study.studyTime << endl;
                    output << NOTIFY_LEVEL_STUDY << " 00080050 ";
                    x_www_form_codec_encode_to_ostream(it_clc->pnfc->study.accessionNumber, &output);
                    output << endl;
                    output << NOTIFY_LEVEL_SERIES << " 00080060 " << it_clc->pnfc->series.modality << endl;
                    string notify = output.str();
                    output.str("");

                    char notify_file_name[MAX_PATH];
                    size_t pos = in_process_sequence_dll(notify_file_name, sizeof(notify_file_name), STATE_DIR);
                    sprintf_s(notify_file_name + pos, sizeof(notify_file_name) - pos, "_%s.dfc", NOTIFY_ACKN_TAG);
                    ofstream ntf(notify_file_name, ios_base::trunc | ios_base::out);
                    if(ntf.good())
                    {
                        ntf << notify ;
                        ntf.close();
                    }
                    else time_header_out(*pflog) << notify;

                    // this instance is all OK, erase it from queue
                    phs->list_action.erase(it_clc);
                }
                else
                {
                    time_header_out(*pflog) << __FUNCSIG__ " NamePipe read file's name is not in queue: " << lpPipeInst->chBuffer << endl;
                }
            }

            // send next action
            if(phs->list_action.empty())
            {
                // no more file in queue, add to lpPipeInst list_blocked_pipe_instances.
                // queue has blocked until new action arriving.
                phs->blocked_pipe_context = lpPipeInst;
            }
            else
            {
                DWORD gle = 0;
                list<action_from_association>::iterator it = phs->list_action.begin();
                while(it != phs->list_action.end())
                {
                    bool wait_response = false;
                    switch(it->type)
                    {
                    case ACTION_TYPE::INDEX_INSTANCE:
                        lpPipeInst->cbShouldWrite = sprintf_s(lpPipeInst->chBuffer, "%s|%s", it->pnfc->file.studyUID, it->pnfc->file.unique_filename);
                        //replace(lpPipeInst->chBuffer, lpPipeInst->chBuffer + strlen(lpPipeInst->chBuffer), '/', '\\');

                        if(!WriteFileEx(lpPipeInst->hPipeInst, lpPipeInst->chBuffer, lpPipeInst->cbShouldWrite, 
                            (LPOVERLAPPED) lpPipeInst, (LPOVERLAPPED_COMPLETION_ROUTINE)write_pipe_complete_func_ptr))
                        {
                            gle = displayErrorToCerr(__FUNCSIG__ " WriteFileEx()", GetLastError(), pflog);
                            disconnect_connection(lpPipeInst);
                        }
                        wait_response = true;
                        ++it;
                        break;
                    case ACTION_TYPE::BURN_PER_STUDY_RELEASE:
                    case ACTION_TYPE::BURN_PER_STUDY_ABORT:
                        // todo: process association disconnect action
                        // if(assoc disconnect && list_action.empty() && assoc lock is empty) exit
                        it = phs->list_action.erase(it);
                        break;
                    default:
                        it = phs->list_action.erase(it);
                        break;
                    }
                    if(wait_response) break;
                }
            }
        }
        else
        {
            time_header_out(*pflog) << __FUNCSIG__ " can't find study_uid " << study_uid << endl;
            disconnect_connection(lpPipeInst);
        }
    }
    else // dwErr != 0
    {
        displayErrorToCerr(__FUNCSIG__ " param dwErr is an error", dwErr, pflog);
        disconnect_connection(lpPipeInst);
    }
}

DWORD named_pipe_server::pipe_client_connect_incoming()
{
    DWORD gle = 0, cbRet = 0;
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, FALSE))
    {
        LPPIPEINST lpPipeInst = new PIPEINST;
        memset(lpPipeInst, 0, sizeof(PIPEINST));
        lpPipeInst->hPipeInst = hPipe; // save hPipe to named pipe instance
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
            opt_verbose ? "-v" : "", study_uid.c_str(), study_uid.c_str(), study_uid.c_str());

        phs = new handle_study(dicomdir, cmd, "dcmmkdir", dicomdir, study_uid);
        if(phs)
        {
            if(0 == phs->start_process(*pflog)) map_study[study_uid] = phs;
            else { delete phs; phs == NULL; }
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
    for_each(map_study.begin(), map_study.end(), [pflog2](const STUDY_PAIR &p) { p.second->print_state(*pflog2); });
}
