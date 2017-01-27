#include <windows.h>
#include "named_pipe_listener.h"
#include <commonlib.h>
extern int opt_verbose;

using namespace std;
using namespace handle_context;

LISTENER_MAP named_pipe_listener::servers;

named_pipe_listener& named_pipe_listener::operator=(const named_pipe_listener &r)
{
    base_path::operator=(r);
    pflog = r.pflog;
    hPipeEvent = r.hPipeEvent;
    hPipe = r.hPipe;
    write_buff_size = r.write_buff_size;
    read_buff_size = r.read_buff_size;
    olPipeConnectOnly = r.olPipeConnectOnly;
    callback_bind_connection = r.callback_bind_connection;
    return *this;
}

named_pipe_listener::~named_pipe_listener(void)
{
    if(opt_verbose) time_header_out(*pflog) << "named_pipe_listener::~named_pipe_listener(" << get_path() << ")" << endl;
    if(hPipe && hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    hPipe = NULL;
    ostream *plog = get_err_stream();
    for_each(map_connections_read.begin(), map_connections_read.end(), [plog](const SHARED_CONN_PAIR &p) {
        if(p.second)
        {
            *plog << "closing map_connections_read[" << p.second->get_id() << "]..." << endl;
            p.second->close_pipe();
        }
    });
    for_each(map_connections_write.begin(), map_connections_write.end(), [plog](const SHARED_CONN_PAIR &p) {
        if(p.second)
        {
            *plog << "closing map_connections_write[" << p.second->get_id() << "]..." << endl;
            p.second->close_pipe();
        }
    });

    for(int i = 0; i < 100 && (map_connections_read.size() || map_connections_write.size()); ++i) SleepEx(10, TRUE);

    SHARED_CONN_MAP::iterator it = map_connections_read.begin();
    while(it != map_connections_read.end())
    {
        time_header_out(*pflog) << "~named_pipe_listener() mandatory remove map_connections_read[" << it->second << "]" << endl;
        if(it->second) it->second->print_state();
        it = map_connections_read.erase(it);
    }
    it = map_connections_write.begin();
    while(it != map_connections_write.end())
    {
        time_header_out(*pflog) << "~named_pipe_listener() mandatory remove map_connections_write[" << it->second << "]" << endl;
        if(it->second) it->second->print_state();
        it = map_connections_write.erase(it);
    }

    servers.erase(hPipeEvent);
    if(hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipeEvent);
        hPipeEvent = NULL;
    }
    if(opt_verbose) time_header_out(*pflog) << "~named_pipe_listener() exit" << endl;
}

void named_pipe_listener::print_state(void) const
{
    *pflog << "named_pipe_listener::print_state() hPipeEvent: " << hPipeEvent << endl
        << "\thPipe: " << hPipe << endl
        << "\tmap_connections_read.size: " << map_connections_read.size() << endl
        << "\tmap_connections_write.size: " << map_connections_write.size() << endl;
    base_path::print_state();
}

DWORD named_pipe_listener::start_listening()
{
    hPipe = INVALID_HANDLE_VALUE;
    if(!hPipeEvent) hPipeEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if(hPipeEvent == NULL)
        return displayErrorToCerr(__FUNCSIG__ " CreateEvent()", GetLastError(), pflog);
    olPipeConnectOnly.hEvent = hPipeEvent;

    string pipe_path(get_path());
    hPipe = CreateNamedPipe(pipe_path.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // message-type pipe, message read mode, blocking mode
        PIPE_UNLIMITED_INSTANCES, write_buff_size, read_buff_size, // output, input buffer size 
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
    if(gle == ERROR_IO_PENDING || gle == ERROR_PIPE_CONNECTED)
        register_named_pipe_listener(this);
    return gle;
}

DWORD named_pipe_listener::pipe_client_connect_incoming()
{
    DWORD gle = 0, cbRet = 0;
    shared_ptr<named_pipe_connection> pnpc;
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, TRUE))
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" async operation complete" << endl;
        ULONG clientProcId = 0;
        GetNamedPipeClientProcessId(hPipe, &clientProcId);
        // bind hPipe to named pipe instance
        pnpc = callback_bind_connection(this, clientProcId);
        if(pnpc)
        {
            if(opt_verbose) time_header_out(*pflog) << "named_pipe_listener::pipe_client_connect_incoming() callback_bind_connection("
                << hex << uppercase << hPipe << ", " << dec << clientProcId << ") OK." << endl;
            map_connections_read[pnpc->get_overlap_read()] = pnpc;
            map_connections_write[pnpc->get_overlap_write()] = pnpc;
            if(opt_verbose) time_header_out(*pflog) << "named_pipe_listener::pipe_client_connect_incoming() add "
                << pnpc->get_overlap_read() << ", " << pnpc->get_overlap_write() << endl;
        }
        else
        {
            ostringstream msg;
            msg << __FUNCSIG__ " callback_bind_connection(" << hex << uppercase << hPipe << ", " << dec << clientProcId << ")";
            gle = displayErrorToCerr(msg.str().c_str(), E_POINTER, pflog);
            // discard this corrupt named pipe instance
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        }
    }
    else
    {
        gle = displayErrorToCerr(__FUNCSIG__ " GetOverlappedResult()", GetLastError(), pflog);
        // discard this corrupt named pipe instance
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    DWORD gle2 = start_listening(); // start a new listening named pipe, new hPipe is created
    if(gle2 != ERROR_IO_PENDING && gle2 != ERROR_PIPE_CONNECTED)
        return displayErrorToCerr(__FUNCSIG__ " start_listening()", gle2, pflog);

    if(gle == 0) // bind conn ok
    {
        gle = pnpc->start_working();
        if(gle)
        {
            displayErrorToCerr(__FUNCSIG__ " pnpc->start_working()", gle, pflog);
            pnpc->close_pipe();
            remove_pipe(pnpc);
        }
    }
    return 0;
}

size_t named_pipe_listener::remove_pipe(const shared_ptr<named_pipe_connection> &pnpc)
{
    if(pnpc == NULL) return 0;
    SHARED_CONN_MAP::size_type removed = map_connections_read.erase(pnpc->get_overlap_read());
    removed += map_connections_write.erase(pnpc->get_overlap_write());
    if(opt_verbose && removed) time_header_out(*pflog) << "named_pipe_listener::remove_pipe(" << pnpc->get_id() << ")."
        << removed << " : " << pnpc->get_overlap_read() << ", " << pnpc->get_overlap_write() << endl;
    return removed;
}

std::ostream* named_pipe_listener::find_err_log()
{
    for(LISTENER_MAP::const_iterator it = servers.cbegin(); it != servers.end(); ++it)
    {
        if(it->second) return it->second->get_err_stream();
    }
    return NULL;
}

shared_ptr<named_pipe_connection> named_pipe_listener::find_connections(std::function<bool(const shared_ptr<named_pipe_connection>&)> pred)
{
    for(SHARED_CONN_MAP::const_iterator it = map_connections_read.cbegin(); it != map_connections_read.cend(); ++it)
    {
        if(it->second == NULL) continue;
        if(pred(it->second)) return it->second;
    }
    return NULL;
}

void CALLBACK named_pipe_listener::remove_closed_connection(ULONG_PTR wParam)
{
    if(wParam == NULL) return;
    named_pipe_connection *pnpc = reinterpret_cast<named_pipe_connection*>(wParam);
    if(pnpc->is_dead())
    {
        if(pnpc->get_event_handle())
        {   // server connection
            named_pipe_listener *pnps = pnpc->get_listener();
            if(pnps)
            {
                shared_ptr<named_pipe_connection> sp = pnps->find_connections_read(pnpc->get_overlap_read());
                if(sp == NULL) sp = pnps->find_connections_write(pnpc->get_overlap_write());
                if(sp) pnps->remove_pipe(sp);
                time_header_out(*pnps->get_err_stream()) << "named_pipe_listener::remove_closed_connection(" << sp->get_id() << ")." << endl;
            }
            else time_header_out(*pnps->get_err_stream()) << "named_pipe_listener::remove_closed_connection(" << pnpc->get_id() << ") error: can't find named_pipe_listener." << endl;
        }
        else delete pnpc; // client connection
    }
}

ostream* handle_context::find_err_log_all()
{
    ostream *plog = named_pipe_listener::find_err_log();
    if(plog) return plog;
    plog = named_pipe_connection::find_err_log_from_alone();
    if(plog) return plog;
    return &cerr;
}

static shared_ptr<named_pipe_connection> find_server_named_pipe_connection(LPOVERLAPPED lpOverLap, named_pipe_listener **ppnps, bool is_write)
{
    ostream* perrlog = NULL;
    if(lpOverLap == NULL)
    {
        if(perrlog == NULL) perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " lpOverLap is NULL." << endl;
        return NULL;
    }
    if(lpOverLap->hEvent == NULL)
    {
        if(perrlog == NULL) perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " lpOverLap->hEvent is NULL, client named_pipe_connection." << endl;
        return NULL; // client named_pipe_connection
    }

    named_pipe_listener *pnps = named_pipe_listener::find_named_pipe_listener(lpOverLap->hEvent);
    if(pnps == NULL)
    {
        if(perrlog == NULL) perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " can't find named_pipe_listener from lpOverLap " << lpOverLap << " -> " << lpOverLap->hEvent << endl;
        return NULL;
    }
    if(ppnps) *ppnps = pnps;
    shared_ptr<named_pipe_connection> npc = is_write ? pnps->find_connections_write(lpOverLap) : pnps->find_connections_read(lpOverLap);
    if(npc == NULL)
    {
        if(perrlog == NULL) perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " can't find named_pipe_connection from lpOverLap " << (is_write ? "write " : "read ") << lpOverLap << endl;
        return NULL;
    }
    return npc;
}

void CALLBACK handle_context::read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    ostream* perrlog = NULL;
    named_pipe_listener *pnps = NULL;
    if(lpOverLap == NULL) return;

    if(lpOverLap->hEvent)
    {
        shared_ptr<named_pipe_connection> pnpc(find_server_named_pipe_connection(lpOverLap, &pnps, false));
        if(pnpc == NULL)
        {
            if(perrlog == NULL) perrlog = find_err_log_all();
            time_header_out(*perrlog) << __FUNCSIG__ " find named_pipe_connection(read) failed: " << lpOverLap << endl;
            return;
        }
        else perrlog = pnpc->get_err_stream();

        if(pnpc->read_pipe_complete(dwErr, cbBytesRead, lpOverLap)) pnpc->close_pipe();
    }
    else
    {
        named_pipe_connection *pnpc = named_pipe_connection::find_alone_connection(lpOverLap, false);
        if(pnpc == NULL)
        {
            if(perrlog == NULL) perrlog = find_err_log_all();
            time_header_out(*perrlog) << __FUNCSIG__ " find named_pipe_connection(read) failed: " << lpOverLap << endl;
            return;
        }
        else perrlog = pnpc->get_err_stream();

        if(pnpc->read_pipe_complete(dwErr, cbBytesRead, lpOverLap)) pnpc->close_pipe();
    }
}

void CALLBACK handle_context::write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    ostream* perrlog = NULL;
    named_pipe_listener *pnps = NULL;
    if(lpOverLap == NULL) return;

    if(lpOverLap->hEvent)
    {
        shared_ptr<named_pipe_connection> pnpc(find_server_named_pipe_connection(lpOverLap, &pnps, true));
        if(pnpc == NULL)
        {
            if(perrlog == NULL) perrlog = find_err_log_all();
            time_header_out(*perrlog) << __FUNCSIG__ " find server named_pipe_connection(write) failed: " << lpOverLap << endl;
            return;
        }
        else perrlog = pnpc->get_err_stream();

        if(pnpc->write_pipe_complete(dwErr, cbBytesWrite, lpOverLap)) pnpc->close_pipe();
    }
    else
    {
        named_pipe_connection *pnpc = named_pipe_connection::find_alone_connection(lpOverLap, true);
        if(pnpc == NULL)
        {
            if(perrlog == NULL) perrlog = find_err_log_all();
            time_header_out(*perrlog) << __FUNCSIG__ " find alone named_pipe_connection(write) failed: " << lpOverLap << endl;
            return;
        }
        else perrlog = pnpc->get_err_stream();

        if(pnpc->write_pipe_complete(dwErr, cbBytesWrite, lpOverLap)) pnpc->close_pipe();
    }
}
