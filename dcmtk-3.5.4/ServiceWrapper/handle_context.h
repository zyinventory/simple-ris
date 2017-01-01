#pragma once

#ifndef _HANDLE_CLASS_
#define _HANDLE_CLASS_

namespace handle_context
{
    class np_conn_assoc_dir : public named_pipe_connection
    {
    private:
        std::string work_path, calling, called, remote, port, transfer_syntax, auto_publish;
        DWORD pid;
        bool assoc_disconn, disconn_release;

        DWORD process_file_incoming(char *assoc_id);
        DWORD establish_conn_dir(char *assoc_id);
        DWORD release_conn_dir(char *assoc_id);

    public:
        np_conn_assoc_dir(named_pipe_listener *pnps) : named_pipe_connection(pnps), pid(0), assoc_disconn(true), disconn_release(false) { };
        virtual ~np_conn_assoc_dir();
        virtual void print_state() const;
        virtual DWORD process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size);
        const char* close_description() const;
        virtual bool is_time_out() const;
    };

}

#endif
