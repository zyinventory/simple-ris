#include "stdafx.h"

#ifndef _ATL_STATIC_REGISTRY // defined in ServiceWrapper
#include <windows.h>
#include <set>
#include <algorithm>
extern int opt_verbose;
#endif

#include "named_pipe_listener.h"
#include <commonlib.h>

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
    connect_callback = r.connect_callback;
    return *this;
}

named_pipe_listener::~named_pipe_listener(void)
{
    if(hPipe && hPipe != INVALID_HANDLE_VALUE) CloseHandle(hPipe);
    hPipe = NULL;
    if(hPipeEvent && hPipeEvent != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hPipeEvent);
        hPipeEvent = NULL;
    }
    if(opt_verbose) time_header_out(*pflog) << "named_pipe_listener::~named_pipe_listener()" << endl;
    
    ostream *plog = get_err_stream();
    set<named_pipe_connection*> ps;
    for_each(map_connections_read.begin(), map_connections_read.end(), [plog, &ps](const CONN_PAIR &p) {
        if(p.second)
        {
            *plog << "map_connections_read : " << p.second->get_handle() << endl;
            ps.insert(p.second);
        }
    });
    for_each(map_connections_write.begin(), map_connections_write.end(), [plog, &ps](const CONN_PAIR &p) {
        if(p.second)
        {
            *plog << "map_connections_write: " << p.second->get_handle() << endl;
            ps.insert(p.second);
        }
    });
    time_header_out(*pflog) << "named_pipe_listener::~named_pipe_listener() start closing " << ps.size() << " connections..." << endl;
    named_pipe_listener *plsnr = this;
    for_each(ps.begin(), ps.end(), [plsnr](named_pipe_connection *p) {
        p->print_state();
        plsnr->remove_pipe(p);
    });
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
    named_pipe_connection *pnpc;
    if(GetOverlappedResult(hPipe, &olPipeConnectOnly, &cbRet, TRUE))
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__" async operation complete" << endl;
        // bind hPipe to named pipe instance
        pnpc = connect_callback(this);
        map_connections_read[pnpc->get_overlap_read()] = pnpc;
        map_connections_write[pnpc->get_overlap_write()] = pnpc;
    }
    else
    {
        gle = displayErrorToCerr(__FUNCSIG__ " GetOverlappedResult()", GetLastError(), pflog);
        // discard this corrupt named pipe instance
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }

    gle = start_listening(); // start a new listening named pipe, hPipe is changed
    if(gle != ERROR_IO_PENDING && gle != ERROR_PIPE_CONNECTED)
        return displayErrorToCerr(__FUNCSIG__ " start_listening()", gle, pflog);

    gle = pnpc->start_working();
    if(gle)
    {
        displayErrorToCerr(__FUNCSIG__ " pnpc->start_working()", gle, pflog);
        pnpc->close_pipe();
        remove_pipe(pnpc);
    }
    return 0;
}

void named_pipe_listener::remove_pipe(named_pipe_connection *pnpc)
{
    if(pnpc == NULL)
    {
        time_header_out(*pflog) << __FUNCSIG__ " connection ptr is NULL." << endl;
        return;
    }
    time_header_out(*pflog) << "remove pipe " << pnpc->get_path() << " : " << pnpc->get_handle() << endl;
    pnpc->print_state();
    map_connections_read.erase(pnpc->get_overlap_read());
    map_connections_write.erase(pnpc->get_overlap_write());
    delete pnpc;
}

std::ostream* named_pipe_listener::find_err_log()
{
    for(LISTENER_MAP::const_iterator it = servers.cbegin(); it != servers.end(); ++it)
    {
        if(it->second) return it->second->get_err_stream();
    }
    return NULL;
}

static ostream* find_err_log_all()
{
    ostream *plog = named_pipe_listener::find_err_log();
    if(plog) return plog;
    plog = named_pipe_connection::find_err_log_from_alone();
    if(plog) return plog;
    return &cerr;
}

static named_pipe_connection* find_named_pipe_connection(LPOVERLAPPED lpOverLap, named_pipe_listener **ppnps, bool is_write)
{
    ostream* perrlog = find_err_log_all();
    if(lpOverLap == NULL)
    {
        time_header_out(*perrlog) << __FUNCSIG__ " lpOverLap is NULL." << endl;
        return NULL;
    }

    if(lpOverLap->hEvent == NULL)
        return named_pipe_connection::find_alone_connection(lpOverLap, is_write);

    named_pipe_listener *pnps = named_pipe_listener::find_named_pipe_listener(lpOverLap->hEvent);
    if(pnps == NULL)
    {
        time_header_out(*perrlog) << __FUNCSIG__ " can't find named_pipe_listener from lpOverLap " << lpOverLap << " -> " << lpOverLap->hEvent << endl;
        return NULL;
    }
    named_pipe_connection *npc = is_write ? pnps->find_connections_write(lpOverLap) : pnps->find_connections_read(lpOverLap);
    if(npc == NULL)
    {
        time_header_out(*perrlog) << __FUNCSIG__ " can't find named_pipe_connection from lpOverLap " << (is_write ? "write " : "read ") << lpOverLap << endl;
        return NULL;
    }
    if(ppnps) *ppnps = pnps;
    return npc;
}

void CALLBACK handle_context::read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    ostream* perrlog = NULL;
    named_pipe_listener *pnps = NULL;
    named_pipe_connection *pnpc = NULL;
    if(lpOverLap->hEvent)
        pnpc = find_named_pipe_connection(lpOverLap, &pnps, false);
    else
        pnpc = named_pipe_connection::find_alone_connection(lpOverLap, false);

    if(pnpc == NULL)
    {
        perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " find named_pipe_connection(read) failed: " << lpOverLap << endl;
        return;
    }
    else perrlog = pnpc->get_err_stream();

    if(pnpc->read_pipe_complete(dwErr, cbBytesRead, lpOverLap))
    {
        if(pnpc->close_pipe())
        {
            time_header_out(*perrlog) << "handle_context::read_pipe_complete() QueueUserAPC(handle_context::remove_named_pipe)" << endl;
            QueueUserAPC(remove_named_pipe, GetCurrentThread(), reinterpret_cast<ULONG_PTR>(pnpc));
        }
    }
}

void CALLBACK handle_context::write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    ostream* perrlog = NULL;
    named_pipe_listener *pnps = NULL;
    named_pipe_connection *pnpc = NULL;
    if(lpOverLap->hEvent)
        pnpc = find_named_pipe_connection(lpOverLap, &pnps, true);
    else
        pnpc = named_pipe_connection::find_alone_connection(lpOverLap, true);
    
    if(pnpc == NULL)
    {
        perrlog = find_err_log_all();
        time_header_out(*perrlog) << __FUNCSIG__ " find named_pipe_connection(write) failed: " << lpOverLap << endl;
        return;
    }
    else perrlog = pnpc->get_err_stream();

    if(pnpc->write_pipe_complete(dwErr, cbBytesWrite, lpOverLap))
    {
        if(pnpc->close_pipe())
        {
            time_header_out(*perrlog) << "handle_context::write_pipe_complete() QueueUserAPC(handle_context::remove_named_pipe)" << endl;
            QueueUserAPC(remove_named_pipe, GetCurrentThread(), reinterpret_cast<ULONG_PTR>(pnpc));
        }
    }
}

void CALLBACK handle_context::remove_named_pipe(ULONG_PTR dwParam)
{
    if(dwParam == NULL) return;
    named_pipe_connection *pnpc = reinterpret_cast<named_pipe_connection*>(dwParam);
    named_pipe_listener *pnps = pnpc->get_listener();
    if(pnps)
    {
        time_header_out(*pnps->get_err_stream()) << "handle_context::remove_named_pipe()" << endl;
        pnps->remove_pipe(pnpc);
    }
    else
    {
        time_header_out(*pnpc->get_err_stream()) << "handle_context::remove_named_pipe(alone_connection)" << endl;
        named_pipe_connection::remove_alone_connection(pnpc);
    }
}
