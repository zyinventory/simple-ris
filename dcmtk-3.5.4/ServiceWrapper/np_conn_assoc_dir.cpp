#include "stdafx.h"

using namespace std;
using namespace handle_context;

const char* np_conn_assoc_dir::close_description() const
{
    if(assoc_disconn)
    {
        if(disconn_release) return "OK";
        else return "ABORT";
    }
    //else if(is_time_out()) return "TIMEOUT";
    else return "UNKNOWN";
}

bool np_conn_assoc_dir::is_time_out() const
{
    time_t timeout, now = time(&now);
    timeout = now - get_last_access();
    if(timeout > assoc_timeout) return true;
    return false;
}

np_conn_assoc_dir::~np_conn_assoc_dir()
{
    if(get_meta_notify_filename().length())
    {
        char newname[MAX_PATH];
        strcpy_s(newname, get_meta_notify_filename().c_str());
        char *p = strrchr(newname, '.');
        if(p)
        {
            *++p = '\0';
            strcat_s(newname, "txt");
            ofstream ofs_txt(newname);
            if(ofs_txt.fail())
            {
                char msg[1024];
                strerror_s(msg, errno);
                time_header_out(*pflog) << __FUNCSIG__" create complete file " << newname << " failed: " << msg << endl;
            }
            else
            {
                ofs_txt << close_description() << endl;
                ofs_txt.close();
            }
        }
    }
}

void np_conn_assoc_dir::print_state() const
{
    *pflog << "np_conn_assoc_dir::print_state() id: " << get_id() << endl
        << "\twork_path: " << work_path << endl
        << "\tcalling: " << calling << endl
        << "\tremote: " << remote << endl
        << "\tcalled: " << called << endl
        << "\ttransfer_syntax: " << transfer_syntax << endl
        << "\tauto_publish: " << auto_publish << endl
        << "\tport: " << dec << port << endl
        << "\tassoc_disconn: " << assoc_disconn << endl
        << "\tdisconn_release: " << disconn_release << endl;
    named_pipe_connection::print_state();
}

DWORD np_conn_assoc_dir::process_message(char *ptr_data_buffer, size_t cbBytesRead, size_t data_buffer_size)
{
    if(opt_verbose && cbBytesRead) time_header_out(*pflog) << "np_conn_assoc_dir::process_message(): " << ptr_data_buffer << endl;
    else time_header_out(*pflog) << "np_conn_assoc_dir::process_message(): cbBytesRead is 0" << endl;

    // process message, must retrieve ack message unless ptr_data_buffer == "close_pipe"
    // if(gle) void CALLBACK handle_context::read_pipe_complete(...) shall close pipe
    if(strncmp(ptr_data_buffer, COMMAND_CLOSE_PIPE, sizeof(COMMAND_CLOSE_PIPE) - 1) == 0) return ERROR_HANDLE_EOF;

    char *assoc_id = strchr(ptr_data_buffer, ' ');
    if(assoc_id == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_message() receive an unknown message: no assoc id." << endl;
        return 0;
    }
    *assoc_id++ = '\0';
    
    if(strncmp(ptr_data_buffer, NOTIFY_FILE_TAG, strlen(NOTIFY_FILE_TAG)) == 0)
        process_file_incoming(assoc_id);
    else if(strncmp(ptr_data_buffer, COMMAND_ASSOC_BEGIN, sizeof(COMMAND_ASSOC_BEGIN) - 1) == 0)
        establish_conn_dir(assoc_id);
    else if(strncmp(ptr_data_buffer, COMMAND_ASSOC_END, sizeof(COMMAND_ASSOC_END) - 1) == 0)
        release_conn_dir(assoc_id); // client shall send close_pipe soon
    else
        time_header_out(*pflog) << "np_conn_assoc_dir::process_message() receive an unknown message: " << ptr_data_buffer << " " << assoc_id << endl;
    return 0;
}

DWORD np_conn_assoc_dir::establish_conn_dir(char *p_assoc_id)
{
    char *notify_file = strchr(p_assoc_id, ' ');
    if(notify_file == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::release_conn_dir() receive an unknown message: no notify file." << endl;
        return ERROR_INVALID_DATA;
    }
    else *notify_file++ = '\0';

    string mnf(GetPacsTemp());
    mnf.append("\\pacs\\"NOTIFY_BASE"\\").append(notify_file);
#ifdef _DEBUG
    time_header_out(cerr) << "np_conn_assoc_dir::process_meta_notify_file() process meta notify: " << mnf << endl;
#endif
    ifstream ntff(mnf, ios_base::in, _SH_DENYWR);
    if(ntff.fail())
    {
        DWORD gle = GetLastError();
        string msg("np_conn_assoc_dir::process_meta_notify_file() open file ");
        msg.append(mnf);
        displayErrorToCerr(msg.c_str(), gle, pflog);
        return ERROR_INVALID_ACCESS;
    }
    string cmd, assoc_id;
    DWORD tag, gle = 0;
    ntff >> cmd >> hex >> tag >> work_path >> dec >> pid >> assoc_id >> calling >> remote >> called >> port >> transfer_syntax >> auto_publish;
    if(ntff.is_open()) ntff.close();

    if(assoc_id.compare(p_assoc_id) == 0)
    {
        work_path.insert(0, "\\pacs\\").insert(0, GetPacsTemp());
        set_id(assoc_id);
        set_meta_notify_filename(mnf);
        assoc_disconn = false;

#ifdef _DEBUG
        time_header_out(cerr) << cmd << " " << hex << tag << " " << work_path << " " << dec << pid << " " << assoc_id << " " << calling << " " << remote << " " << called << " " << port << endl;
#endif
        return 0;
    }
    else return ERROR_INVALID_DATA;
}

DWORD np_conn_assoc_dir::release_conn_dir(char *p_assoc_id)
{
#ifdef _DEBUG
    time_header_out(cerr) << "np_conn_assoc_dir::release_conn_dir() receive association release: " << p_assoc_id << endl;
#endif
    char *release_result = strchr(p_assoc_id, ' ');
    if(release_result == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::release_conn_dir() receive an unknown message: no release result." << endl;
        return ERROR_INVALID_DATA;
    }
    else *release_result++ = '\0';

    char *notify_file = strchr(release_result, ' ');
    if(notify_file == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::release_conn_dir() receive an unknown message: no notify file." << endl;
        return ERROR_INVALID_DATA;
    }
    else *notify_file++ = '\0';

    assoc_disconn = true;
    disconn_release = (strncmp(release_result, STORE_RESULT_RELEASE, sizeof(STORE_RESULT_RELEASE) - 1) == 0);
    return 0;
}

DWORD np_conn_assoc_dir::process_file_incoming(char *p_assoc_id)
{
#ifdef _DEBUG
    time_header_out(cerr) << "np_conn_assoc_dir::process_file_incoming() receive file notify: " << p_assoc_id << endl;
#endif
    if(opt_verbose) time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive file notify: " << p_assoc_id << endl;
    char *study_uid = strchr(p_assoc_id, ' ');
    if(study_uid == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_message() receive an unknown file notify: no study uid." << endl;
        return ERROR_INVALID_DATA;
    }
    else *study_uid++ = '\0';

    char *notify_file = strchr(study_uid, ' ');
    if(notify_file == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_message() receive an unknown file notify: no notify file." << endl;
        return ERROR_INVALID_DATA;
    }
    else *notify_file++ = '\0';

    //process_notify(notify_file, compress_queue, *pflog);
    return 0;
}
