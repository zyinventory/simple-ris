#pragma once
#ifndef _ATL_STATIC_REGISTRY
#include <string>
#include <iostream>
#include <list>
#include <map>
#include "dcmtk/dcmdata/notify_context.h"
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
    };

    class named_pipe_connection : public base_dir
    {
    private:
        static CONN_MAP map_alone_connections_read, map_alone_connections_write;
        static std::list<named_pipe_connection*> wait_close_conn_list;
	    OVERLAPPED oOverlap_read, oOverlap_write;
        named_pipe_listener *p_listener;
	    HANDLE hPipeInst;
        size_t bytes_queued, write_buff_size, read_buff_size;
        bool closing, reading, removed_from_map;
        char *ptr_write_buff, *ptr_read_buff;
        std::list<char> message_read_buffer;
        std::list<std::string> message_write_buffer;

        DWORD read_message();
        DWORD write_message(size_t num, const void *ptr_data);
        DWORD write_message_pack();

    public:
        static void regist_alone_connection(named_pipe_connection*);
        static size_t remove_alone_connection(named_pipe_connection*);
        static named_pipe_connection* find_alone_connection(LPOVERLAPPED, bool is_write);
        static void delete_closing_connection();
        static std::ostream* find_err_log_from_alone();
        named_pipe_connection(const char *assoc_id, const char *path, const char *meta_notify_file, size_t wbuff_size, size_t rbuff_size, std::ostream *plog)
            : base_dir(assoc_id, path, meta_notify_file, plog), hPipeInst(NULL), write_buff_size(wbuff_size), read_buff_size(rbuff_size),
            closing(false), reading(false), ptr_write_buff(NULL), ptr_read_buff(NULL), bytes_queued(0), p_listener(NULL), removed_from_map(false)
        {
            memset(&oOverlap_read, 0, sizeof(oOverlap_read));
            memset(&oOverlap_write, 0, sizeof(oOverlap_write));
            ptr_read_buff = new char[read_buff_size + 1];
        };
        named_pipe_connection(named_pipe_listener *pnps) : base_dir("", pnps->get_path().c_str(), "", pnps->get_err_stream()),
            closing(false), reading(false), ptr_write_buff(NULL), ptr_read_buff(NULL), bytes_queued(0), removed_from_map(false), p_listener(pnps),
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
        LPOVERLAPPED get_overlap_read() { return &oOverlap_read; };
        LPOVERLAPPED get_overlap_write() { return &oOverlap_write; };
        size_t get_bytes_queued() { return bytes_queued; };
        size_t get_write_buff_size() const { return write_buff_size; };
        size_t get_read_buff_size() const { return read_buff_size; };
        bool is_write_queue_empty() const { return (bytes_queued == 0 && message_write_buffer.size() == 0); };
        bool close_pipe();
        bool is_closing() { return closing; };
        bool is_reading() { return reading; };
        named_pipe_listener* get_listener() const { return p_listener; };
        
        DWORD queue_message(const std::string &msg);
        DWORD read_pipe_complete(DWORD dwErr, DWORD cbBytesRead, LPOVERLAPPED lpOverLap);
        DWORD write_pipe_complete(DWORD dwErr, DWORD cbBytesWrite, LPOVERLAPPED lpOverLap);
    };
}
