#include "stdafx.h"

#ifndef _ATL_STATIC_REGISTRY // defined in ServiceWrapper
#include <windows.h>
#include <iterator>
#include <sstream>
#include "named_pipe_listener.h"
#include <commonlib.h>
#endif

using namespace std;
using namespace handle_context;

ALONE_CONN_MAP named_pipe_connection::map_alone_connections_read, named_pipe_connection::map_alone_connections_write;

void named_pipe_connection::regist_alone_connection(named_pipe_connection *pnpc)
{
    if(pnpc)
    {
        map_alone_connections_read[pnpc->get_overlap_read()] = pnpc;
        map_alone_connections_write[pnpc->get_overlap_write()] = pnpc;
    }
}

named_pipe_connection* named_pipe_connection::find_alone_connection(LPOVERLAPPED pol, bool is_write)
{
    if(pol == NULL) return NULL;
    if(is_write)
        return map_alone_connections_write.count(pol) ? map_alone_connections_write[pol] : NULL;
    else
        return map_alone_connections_read.count(pol) ? map_alone_connections_read[pol] : NULL;
}

size_t named_pipe_connection::remove_connection(named_pipe_connection *pnpc)
{
    if(pnpc == NULL) return 0;
    SHARED_CONN_MAP::size_type removed = map_alone_connections_read.erase(pnpc->get_overlap_read());
    removed += map_alone_connections_write.erase(pnpc->get_overlap_write());
    if(removed) time_header_out(*find_err_log_all()) << "named_pipe_listener::remove_alone_connection() remove "
        << pnpc->get_overlap_read() << ", " << pnpc->get_overlap_write() << endl;
    return removed;
}

std::ostream* named_pipe_connection::find_err_log_from_alone()
{
    for(ALONE_CONN_MAP::const_iterator it = map_alone_connections_read.cbegin(); it != map_alone_connections_read.end(); ++it)
    {
        if(it->second) return it->second->get_err_stream();
    }
    return NULL;
}

named_pipe_connection::~named_pipe_connection()
{
    for(int i = 0; i < 100 && !close_pipe(); ++i) SleepEx(0, TRUE);
    if(hEvent && !close_pipe()) DisconnectNamedPipe(hPipeInst);
    SleepEx(0, TRUE);
    if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE) CloseHandle(hPipeInst);
    SleepEx(0, TRUE);
    hPipeInst = NULL;
    if(ptr_write_buff) delete ptr_write_buff;
    if(ptr_read_buff) delete ptr_read_buff;
    if(opt_verbose) time_header_out(*pflog) << "~named_pipe_connection(" << get_id() << ") ok." << endl;
}

DWORD named_pipe_connection::start_working()
{
    // server connection
    if(hEvent)
    {
        callback_pipe_connected();
        return read_message();
    }
    // client connection
    DWORD gle = 0;
    bool pipeStandby = false;
    for(int i = 0; i < 100; ++i)
    {
        if(WaitNamedPipe(get_path().c_str(), NMPWAIT_USE_DEFAULT_WAIT))
            pipeStandby = true;
        if(pipeStandby)
        {
            hPipeInst = CreateFile(NAMED_PIPE_QR, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
            if(hPipeInst != INVALID_HANDLE_VALUE) break;
        }
    }
    if(hPipeInst == NULL || hPipeInst == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        return displayErrorToCerr(__FUNCSIG__" can't WaitNamedPipe("NAMED_PIPE_QR")", gle);
    }
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    BOOL fSuccess = SetNamedPipeHandleState(hPipeInst, &dwMode, NULL, NULL);
    if(!fSuccess)
        return displayErrorToCerr(__FUNCSIG__ " SetNamedPipeHandleState(PIPE_READMODE_MESSAGE) failed", GetLastError(), pflog);

    callback_pipe_connected();
    return read_message();
}

bool named_pipe_connection::close_pipe()
{
    if(!closing)
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ " try CancelIo(" << get_id() << ")" << endl;
        closing = true;
        if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE) CancelIo(hPipeInst);
    }

    if(reading || bytes_queued)
    {
        if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ " wait some pending IO..." << endl;
        return false;
    }
    else
    {
        if(hPipeInst)
        {
            if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ " CancelIo OK, DisconnectNamedPipe and CloseHandle " << get_id() << endl;
            if(hEvent) // is server side?
            {
                if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE)
                {
                    if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ " DisconnectNamedPipe(" << get_id() << ")." << endl;
                    if (! DisconnectNamedPipe(hPipeInst))
                        displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError(), pflog);
                }
            }
        
            if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE)
            {
                if(opt_verbose) time_header_out(*pflog) << __FUNCSIG__ " CloseHandle(" << get_id() << ")." << endl;
                CloseHandle(hPipeInst);
            }
            hPipeInst = NULL;
            callback_pipe_closed();
            if(hEvent) QueueUserAPC(named_pipe_listener::remove_closed_connection, GetCurrentThread(), reinterpret_cast<ULONG_PTR>(this));
        }
        return true;
    }
}

void named_pipe_connection::print_state(void) const
{
    *pflog << "named_pipe_connection::print_state() hPipe: " << hPipeInst << endl
        << "\thEvent: " << oOverlap_read.hEvent << endl
        << "\toOverlap_read: " << &oOverlap_read << endl
        << "\toOverlap_write: " << &oOverlap_write << endl
        << "\twrite_buff_size: " << write_buff_size << endl
        << "\tread_buff_size: " << read_buff_size << endl
        << "\tclosing: " << closing << endl
        << "\treading: " << reading << endl
        << "\tbytes_queued: " << bytes_queued << endl;
    base_dir::print_state();
}

DWORD named_pipe_connection::read_message()
{
    if(closing) return ERROR_HANDLE_EOF;
    if(ptr_read_buff == NULL)
    {
        ptr_read_buff = new char[read_buff_size + 1];
        if(ptr_write_buff == NULL)
            return displayErrorToCerr(__FUNCSIG__" malloc read buffer", ERROR_NOT_ENOUGH_MEMORY, pflog);
    }

    refresh_last_access();
    reading = true;
    if(!ReadFileEx(hPipeInst, ptr_read_buff, read_buff_size, &oOverlap_read, handle_context::read_pipe_complete))
    {
        reading = false;
        return displayErrorToCerr(__FUNCSIG__" ReadFileEx()", GetLastError(), pflog);
    }
    return 0;
}

DWORD named_pipe_connection::read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap)
{
    DWORD gle = 0;
    reading = false;
    refresh_last_access();

    if(dwErr == 0)
    {
        if(lpOverLap->Internal)
        {
            gle = GetLastError();
            if(gle == ERROR_IO_PENDING) // fragment message
            {
                copy(ptr_read_buff, ptr_read_buff + cbBytesRead, back_inserter(message_read_buffer));
                gle = 0;
                reading = true;
                if(!ReadFileEx(hPipeInst, ptr_read_buff, read_buff_size, &oOverlap_read, handle_context::read_pipe_complete))
                {
                    reading = false;
                    gle = displayErrorToCerr(__FUNCSIG__" ReadFileEx(ERROR_IO_PENDING)", GetLastError(), pflog);
                }
            }
            else
            {
                displayErrorToCerr(__FUNCSIG__" lpOverLap->Internal", lpOverLap->Internal, pflog);
                displayErrorToCerr(__FUNCSIG__" lpOverLap->Internal", gle, pflog);
            }
            return gle;
        }

        stringstream strstrm;
        if(message_read_buffer.size())
        {
            size_t message_size = message_read_buffer.size();
            char *ptr_message = new char[message_size + cbBytesRead + 1];
            copy(message_read_buffer.cbegin(), message_read_buffer.cend(), ptr_message);
            memcpy_s(ptr_message + message_size, cbBytesRead + 1, ptr_read_buff, cbBytesRead);
            ptr_message[message_size + cbBytesRead] = '\0';
            message_read_buffer.clear();
            strstrm << ptr_message << endl;
            delete ptr_message;
        }
        else
        {
            ptr_read_buff[cbBytesRead] = '\0';
            strstrm << ptr_read_buff << endl;
        }
        
        while(strstrm.getline(ptr_read_buff, read_buff_size).good())
        {
            size_t len = strlen(ptr_read_buff);
            if(len)
            {
                if(strncmp(ptr_read_buff, COMMAND_CLOSE_PIPE, sizeof(COMMAND_CLOSE_PIPE) - 1) == 0)
                {
                    gle = ERROR_HANDLE_EOF;
                    time_header_out(*pflog) << __FUNCSIG__" read close_pipe complete." << endl;
                }
                else gle = process_message(ptr_read_buff, len, len + 1);
                if(gle) break;
            }
        }

        if(gle)
        {
            if(gle != ERROR_HANDLE_EOF) displayErrorToCerr(__FUNCSIG__ " process_message error", gle, pflog);
        }
        else
        {
            if(closing) gle = ERROR_HANDLE_EOF;
            else gle = read_message();
        }
    }
    else
    {
        if(closing && dwErr == ERROR_OPERATION_ABORTED)
            time_header_out(*pflog) << __FUNCSIG__ " peer close pipe." << endl;
        else
            gle = displayErrorToCerr(__FUNCSIG__ " dwErr error", dwErr, pflog);
    }
    return gle;
}

DWORD named_pipe_connection::write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap)
{
    DWORD gle = 0;
    bool shall_close_pipe = false;
    refresh_last_access();

    if (dwErr == 0)
    {
        if(cbBytesWrite)
            time_header_out(*pflog) << "named_pipe_connection::write_pipe_complete(): " << (ptr_write_buff ? ptr_write_buff : "(NULL)") << endl;
        
        shall_close_pipe = (ptr_write_buff && strstr(ptr_write_buff, COMMAND_CLOSE_PIPE));

        bytes_queued = 0;
        if(ptr_write_buff)
        {
            delete ptr_write_buff;
            ptr_write_buff = NULL;
        }

        if(!shall_close_pipe && message_write_buffer.size()) gle = write_message_pack();
    }
    else gle = displayErrorToCerr(__FUNCSIG__, dwErr, pflog);

    if(closing || shall_close_pipe)
    {
        bytes_queued = 0;
        gle = ERROR_HANDLE_EOF;
    }

    return gle;
}

DWORD named_pipe_connection::write_message(size_t num, const void *ptr_data)
{
    if(bytes_queued) return ERROR_PIPE_BUSY;
    if(closing) return ERROR_HANDLE_EOF;
    
    refresh_last_access();

    // packing message, save data to buffer
    bytes_queued = num;
    ptr_write_buff = new char[num + 1];
    if(ptr_write_buff == NULL)
        return displayErrorToCerr(__FUNCSIG__" calloc read buffer", ERROR_NOT_ENOUGH_MEMORY, pflog);
    memcpy_s(ptr_write_buff, bytes_queued + 1, ptr_data, bytes_queued);
    ptr_write_buff[bytes_queued] = '\0';

    if(WriteFileEx(hPipeInst, ptr_write_buff, bytes_queued, &oOverlap_write, handle_context::write_pipe_complete))
        return 0;
    else
        return displayErrorToCerr(__FUNCSIG__" WriteFileEx()", GetLastError(), pflog);
}

DWORD named_pipe_connection::write_message_pack()
{
    size_t pack_size = write_buff_size / 3;
    string pack;
    pack.reserve(pack_size + 1);
    do {
        if(message_write_buffer.front().length())
            pack.append(message_write_buffer.front()).append(1, '\n');
        message_write_buffer.pop_front();
    } while(message_write_buffer.size() && pack.length() + message_write_buffer.front().length() + 1 <= pack_size);
    return write_message(pack.size(), pack.c_str());
}

DWORD named_pipe_connection::queue_message(const std::string &msg)
{
    time_header_out(*pflog) << "named_pipe_connection::queue_message(" << msg << ")" << endl;
    if(bytes_queued)
    {
        message_write_buffer.push_back(msg);
        return 0;
    }
    
    if(message_write_buffer.size())
    {
        message_write_buffer.push_back(msg);
        return write_message_pack();
    }
    else return write_message(msg.size(), msg.c_str());
}
