#include "stdafx.h"

#ifndef _ATL_STATIC_REGISTRY // defined in ServiceWrapper
#include <windows.h>
#include <iterator>
#include <sstream>
#endif

#include "named_pipe_listener.h"
#include <commonlib.h>

using namespace std;
using namespace handle_context;

CONN_MAP named_pipe_connection::map_alone_connections_read, named_pipe_connection::map_alone_connections_write;

void named_pipe_connection::regist_alone_connection(named_pipe_connection *pnpc)
{
    if(pnpc)
    {
        map_alone_connections_read[pnpc->get_overlap_read()] = pnpc;
        map_alone_connections_write[pnpc->get_overlap_write()] = pnpc;
    }
}

void named_pipe_connection::remove_alone_connection(named_pipe_connection *pnpc)
{
    if(pnpc)
    {
        map_alone_connections_read.erase(pnpc->get_overlap_read());
        map_alone_connections_write.erase(pnpc->get_overlap_read());
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

std::ostream* named_pipe_connection::find_err_log_from_alone()
{
    for(CONN_MAP::const_iterator it = map_alone_connections_read.cbegin(); it != map_alone_connections_read.end(); ++it)
    {
        if(it->second) return it->second->get_err_stream();
    }
    return NULL;
}

named_pipe_connection::~named_pipe_connection()
{
    if(ptr_write_buff) delete ptr_write_buff;
    if(ptr_read_buff) delete ptr_read_buff;
    if(!closing) close_pipe();
    if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE) CloseHandle(hPipeInst);
    hPipeInst = NULL;
}

DWORD named_pipe_connection::start_working()
{
    if(p_listener) return read_message();

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
    if(hPipeInst == INVALID_HANDLE_VALUE)
    {
        gle = GetLastError();
        time_header_out(*pflog) << __FUNCSIG__ " can't WaitNamedPipe(" << NAMED_PIPE_QR << ")" << endl;
        return gle;
    }
    DWORD dwMode = PIPE_READMODE_MESSAGE;
    BOOL fSuccess = SetNamedPipeHandleState(hPipeInst, &dwMode, NULL, NULL);
    if(!fSuccess)
        return displayErrorToCerr(__FUNCSIG__ " SetNamedPipeHandleState(PIPE_READMODE_MESSAGE) failed", GetLastError(), pflog);

    return read_message();
}

bool named_pipe_connection::close_pipe()
{
    if(!closing)
    {
        closing = true;
        if(p_listener)
        {
            if(hPipeInst && hPipeInst != INVALID_HANDLE_VALUE)
            {
                if (! DisconnectNamedPipe(hPipeInst))
                    displayErrorToCerr(__FUNCSIG__ " DisconnectNamedPipe()", GetLastError(), pflog);
            }
            else time_header_out(*pflog) << __FUNCSIG__ " hPipeInst is invalid." << endl;
        }
    }
    if(reading || bytes_queued) return false;
    else return true;
}

void named_pipe_connection::print_state(void) const
{
    *pflog << "named_pipe_connection::print_state() hPipe: " << hPipeInst << endl
        << "\thEvent: " << oOverlap_read.hEvent << endl
        << "\toOverlap_read: " << &oOverlap_read << endl
        << "\toOverlap_write: " << &oOverlap_write << endl
        << "\twrite_buff_size: " << write_buff_size << endl
        << "\tread_buff_size: " << read_buff_size << endl;
    named_pipe_base::print_state();
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

    if(dwErr == 0)
    {
        if(lpOverLap->Internal)
        {
            gle = GetLastError();
            if(gle == ERROR_IO_PENDING)
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
                if(strncmp(ptr_read_buff, "close_pipe", 10) == 0)
                {
                    gle = ERROR_HANDLE_EOF;
                    time_header_out(*pflog) << __FUNCSIG__" read close_pipe complete." << endl;
                }
                else
                    gle = process_message(ptr_read_buff, len, len + 1);
                if(gle) break;
            }
        }

        if(gle)
        {
            if(gle == ERROR_HANDLE_EOF) return gle;
            else return displayErrorToCerr(__FUNCSIG__ " process_message error", gle, pflog);
        }
        else
        {
            if(closing) return ERROR_HANDLE_EOF;
            else return read_message();
        }
    }
    else
    {
        if(closing && dwErr == ERROR_PIPE_NOT_CONNECTED)
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
    if (dwErr == 0)
    {
        if(cbBytesWrite)
            time_header_out(*pflog) << "named_pipe_connection::write_pipe_complete(): " << (ptr_write_buff ? ptr_write_buff : "(NULL)") << endl;
        
        shall_close_pipe = (ptr_write_buff && strstr(ptr_write_buff, "close_pipe"));

        bytes_queued = 0;
        if(ptr_write_buff)
        {
            delete ptr_write_buff;
            ptr_write_buff = NULL;
        }

        if(shall_close_pipe) gle = ERROR_HANDLE_EOF;
        else if(message_write_buffer.size()) gle = write_message_pack();
    }
    else gle = displayErrorToCerr(__FUNCSIG__, dwErr, pflog);

    if(closing)
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
