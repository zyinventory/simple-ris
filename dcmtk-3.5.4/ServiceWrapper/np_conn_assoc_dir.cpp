#include "stdafx.h"

using namespace std;
using namespace handle_context;

const char* np_conn_assoc_dir::close_description() const
{
    if(is_disconn())
    {
        if(disconn_release) return "OK";
        else return "ABORT";
    }
    //else if(is_time_out(assoc_timeout)) return "TIMEOUT";
    else return "CONNECTED";
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
        << "\tcalling: " << calling << endl
        << "\tremote: " << remote << endl
        << "\tcalled: " << called << endl
        << "\ttransfer_syntax: " << transfer_syntax << endl
        << "\tauto_publish: " << auto_publish << endl
        << "\tport: " << dec << port << endl
        << "\tis_disconn: " << is_disconn() << endl
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

// STOR_BEG | assoc_id assoc_path notify_file pid callingAE remoteHostName calledAE port xfer auto_publish
DWORD np_conn_assoc_dir::establish_conn_dir(const char *p_assoc_id)
{
    string assoc_id, store_path, notify_file;
    istringstream istrm(p_assoc_id);
    istrm >> assoc_id >> store_path >> notify_file >> dec >> pid >> calling >> remote >> called >> port >> transfer_syntax >> auto_publish;

    if(assoc_id.compare(p_assoc_id) == 0)
    {
        set_path(store_path);
        set_id(assoc_id);
        set_meta_notify_filename(notify_file);
#ifdef _DEBUG
        time_header_out(cerr) << "np_conn_assoc_dir::establish_conn_dir() receive: " << p_assoc_id << endl;
#endif
        return 0;
    }
    else return displayErrorToCerr("np_conn_assoc_dir::establish_conn_dir() assoc id match", ERROR_INVALID_DATA, pflog);
}

// STOR_END | assoc_id RELEASE notify_filename
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

    disconn_release = (strncmp(release_result, STORE_RESULT_RELEASE, sizeof(STORE_RESULT_RELEASE) - 1) == 0);
    return 0;
}

// FILE | assoc_id hash study_uid unique_filename notify_filename instance_filename
DWORD np_conn_assoc_dir::process_file_incoming(char *p_assoc_id)
{
#ifdef _DEBUG
    time_header_out(cerr) << "np_conn_assoc_dir::process_file_incoming() receive file notify: " << p_assoc_id << endl;
#endif
    if(opt_verbose) time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive file notify: " << p_assoc_id << endl;

    char *hash = strchr(p_assoc_id, ' ');
    if(hash == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive an unknown file notify: no hash." << endl;
        return ERROR_INVALID_DATA;
    }
    else *hash++ = '\0';

    char *study_uid = strchr(hash, ' ');
    if(study_uid == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive an unknown file notify: no study uid." << endl;
        return ERROR_INVALID_DATA;
    }
    else *study_uid++ = '\0';

    char *unique_filename = strchr(study_uid, ' ');
    if(unique_filename == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive an unknown file notify: no unique file name." << endl;
        return ERROR_INVALID_DATA;
    }
    else *unique_filename++ = '\0';

    char *notify_filename = strchr(unique_filename, ' ');
    if(notify_filename == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive an unknown file notify: no notify file." << endl;
        return ERROR_INVALID_DATA;
    }
    else *notify_filename++ = '\0';

    char *instance_filename = strchr(notify_filename, ' ');
    if(instance_filename == NULL)
    {
        time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() receive an unknown file notify: no instance file." << endl;
        return ERROR_INVALID_DATA;
    }
    else *instance_filename++ = '\0';

    if(get_listener())
    {   // server connection shall establish assoc <--> study many to many relationship
        study_assoc_dir* pstudy = NULL;
        STUDY_MAP::iterator it = studies.find(study_uid);
        if(it != studies.end()) pstudy = it->second;
        if(pstudy = NULL)
        {
            char study_path[MAX_PATH];
            int used = sprintf_s(study_path, "%s\\orders_study\\", GetPacsTemp());
            char *orders_study_name = study_path + used;
            used += in_process_sequence_dll(study_path + used, sizeof(study_path) - used, "");
            study_path[used++] = '_';
            strcpy_s(study_path + used, sizeof(study_path) - used, study_uid);
            if(MkdirRecursive(study_path))
            {
                pstudy = study_assoc_dir::create_instance(study_uid, orders_study_name, notify_filename, pflog);
                studies[study_uid] = pstudy;
            }
            else time_header_out(*pflog) << "np_conn_assoc_dir::process_file_incoming() can't create dir " << study_path << endl;
        }
        if(pstudy) pstudy->add_file(this, hash, unique_filename, notify_filename, instance_filename);
    }

    return 0;
}

void np_conn_assoc_dir::fill_association(NOTIFY_FILE_CONTEXT *pnfc) const
{
    strncpy_s(pnfc->src_notify_filename, get_meta_notify_filename().c_str(), _TRUNCATE);
    pnfc->assoc.port = port;
    strncpy_s(pnfc->assoc.id, get_id().c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.store_assoc_id, get_id().c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.path, get_path().c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.calledAE, called.c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.callingAE, calling.c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.callingAddr, remote.c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.calledAE, called.c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.expected_xfer, transfer_syntax.c_str(), _TRUNCATE);
    strncpy_s(pnfc->assoc.auto_publish, auto_publish.c_str(), _TRUNCATE);
}
