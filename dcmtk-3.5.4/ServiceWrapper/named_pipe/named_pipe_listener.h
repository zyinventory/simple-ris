#pragma once
#ifndef _ATL_STATIC_REGISTRY
#include <string>
#include <iostream>
#include <list>
#include <map>
#include <iterator>
#include <tuple>
#include <array>
#include "dcmtk/dcmdata/notify_context.h"
extern int opt_verbose;
#endif

#define NAMED_PIPE_QR "\\\\.\\pipe\\dcmtk_qr"
#define NOTIFY_BASE "store_notify"
#define PIPE_BUFFER_SIZE 4095

namespace handle_context
{
    const char COMMAND_CLOSE_PIPE[] = "close_pipe", COMMAND_ASSOC_BEGIN[] = "STOR_BEG", COMMAND_ASSOC_END[] = "STOR_END",
        STORE_RESULT_RELEASE[] = "RELEASE", STORE_RESULT_ABORT[] = "ABORT";

    class named_pipe_listener;
    class named_pipe_connection;
    typedef named_pipe_connection* (WINAPI *LPPIPE_CONNECT_CALLBACK)(named_pipe_listener*);
    void CALLBACK read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
    void CALLBACK write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);
    std::ostream* find_err_log_all();

    typedef std::map<HANDLE, named_pipe_listener*> LISTENER_MAP;
    typedef std::pair<HANDLE, named_pipe_listener*> LISTENER_PAIR;
    typedef std::map<LPOVERLAPPED, named_pipe_connection*> CONN_MAP;
    typedef std::pair<LPOVERLAPPED, named_pipe_connection*> CONN_PAIR;
    
    class named_pipe_listener : public base_path
    {
    private:
        static LISTENER_MAP servers;
        static void register_named_pipe_listener(named_pipe_listener *p){ servers[p->hPipeEvent] = p; };
        HANDLE hPipeEvent, hPipe; // hPipe will change per listening
        size_t write_buff_size, read_buff_size;
        OVERLAPPED olPipeConnectOnly;
        LPPIPE_CONNECT_CALLBACK connect_callback;
        CONN_MAP map_connections_read, map_connections_write;

    public:
        static named_pipe_listener* find_named_pipe_listener(HANDLE h){ return servers.count(h) ? servers[h] : NULL; };
        static std::ostream* find_err_log();
        named_pipe_listener(const char *pipe_path, DWORD write_size, DWORD read_size, LPPIPE_CONNECT_CALLBACK conn_callback, std::ostream *plog)
            : base_path(pipe_path, plog), hPipeEvent(NULL), hPipe(NULL), write_buff_size(write_size), read_buff_size(read_size), connect_callback(conn_callback)
            { memset(&olPipeConnectOnly, 0, sizeof(OVERLAPPED)); };
        named_pipe_listener(const named_pipe_listener &r) : base_path(r), hPipeEvent(r.hPipeEvent), hPipe(r.hPipe),
            write_buff_size(r.write_buff_size), read_buff_size(r.read_buff_size), olPipeConnectOnly(r.olPipeConnectOnly),
            connect_callback(r.connect_callback){};
        named_pipe_listener& operator=(const named_pipe_listener &r);
        virtual ~named_pipe_listener(void);
        size_t get_write_buff_size() const { return write_buff_size; };
        size_t get_read_buff_size() const { return read_buff_size; };
        DWORD start_listening();
        DWORD pipe_client_connect_incoming();
        size_t remove_pipe(named_pipe_connection *pnpc);
        named_pipe_connection* find_connections_read(LPOVERLAPPED lpo) { return map_connections_read.count(lpo) ? map_connections_read[lpo] : NULL; };
        named_pipe_connection* find_connections_write(LPOVERLAPPED lpo) { return map_connections_write.count(lpo) ? map_connections_write[lpo] : NULL; };
        virtual void print_state(void) const;
        virtual HANDLE get_handle() const { return hPipeEvent; };
        HANDLE get_current_pipe_handle() const { return hPipe; };
        void detect_timeout_connection();
        const named_pipe_connection* find_and_remove_dead_connection();
    };

    class named_pipe_connection : public base_dir
    {
    private:
        static CONN_MAP map_alone_connections_read, map_alone_connections_write;
	    OVERLAPPED oOverlap_read, oOverlap_write;
        named_pipe_listener *p_listener;
	    HANDLE hPipeInst;
        size_t bytes_queued, write_buff_size, read_buff_size;
        bool closing, reading;
        char *ptr_write_buff, *ptr_read_buff;
        std::list<char> message_read_buffer;
        std::list<std::string> message_write_buffer;

        DWORD read_message();
        DWORD write_message(size_t num, const void *ptr_data);
        DWORD write_message_pack();

    public:
        static void regist_alone_connection(named_pipe_connection*);
        static size_t remove_connection(named_pipe_connection*);
        static named_pipe_connection* find_alone_connection(LPOVERLAPPED, bool is_write);
        //static const named_pipe_connection* find_and_remove_dead_alone_connection();
        //static void close_timeout_alone_connection();
        static std::ostream* find_err_log_from_alone();
        named_pipe_connection(const char *assoc_id, const char *path, const char *meta_notify_file, size_t wbuff_size, size_t rbuff_size, int timeout, std::ostream *plog)
            : base_dir(assoc_id, path, meta_notify_file, timeout, plog), hPipeInst(NULL), write_buff_size(wbuff_size), read_buff_size(rbuff_size),
            closing(false), reading(false), ptr_write_buff(NULL), ptr_read_buff(NULL), bytes_queued(0), p_listener(NULL)
        {
            memset(&oOverlap_read, 0, sizeof(oOverlap_read));
            memset(&oOverlap_write, 0, sizeof(oOverlap_write));
            ptr_read_buff = new char[read_buff_size + 1];
        };
        named_pipe_connection(named_pipe_listener *pnps, int timeout) : base_dir("", pnps->get_path().c_str(), "", timeout, pnps->get_err_stream()),
            closing(false), reading(false), ptr_write_buff(NULL), ptr_read_buff(NULL), bytes_queued(0), p_listener(pnps),
            write_buff_size(pnps->get_write_buff_size()), read_buff_size(pnps->get_read_buff_size()), hPipeInst(pnps->get_current_pipe_handle())
        {
            memset(&oOverlap_read, 0, sizeof(oOverlap_read));
            memset(&oOverlap_write, 0, sizeof(oOverlap_write));
            oOverlap_read.hEvent = pnps->get_handle();
            oOverlap_write.hEvent = pnps->get_handle();
            ptr_read_buff = new char[read_buff_size + 1];
        };
        virtual ~named_pipe_connection();
        virtual void print_state(void) const;
        virtual HANDLE get_handle() const { return hPipeInst; };
        virtual DWORD start_working();
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size) { return 0; };
        virtual void callback_pipe_connected() { };
        virtual void callback_pipe_closed() { };
        LPOVERLAPPED get_overlap_read() { return &oOverlap_read; };
        LPOVERLAPPED get_overlap_write() { return &oOverlap_write; };
        size_t get_bytes_queued() const { return bytes_queued; };
        size_t get_write_buff_size() const { return write_buff_size; };
        size_t get_read_buff_size() const { return read_buff_size; };
        bool is_write_queue_empty() const { return (bytes_queued == 0 && message_write_buffer.size() == 0); };
        bool close_pipe();
        bool is_dead() const { return !(reading || bytes_queued || hPipeInst); };
        bool is_disconn() const { return hPipeInst == NULL; };
        named_pipe_listener* get_listener() const { return p_listener; };
        
        DWORD queue_message(const std::string &msg);
        DWORD read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        DWORD write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);
    };
}
