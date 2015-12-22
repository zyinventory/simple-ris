#include "stdafx.h"
using namespace std;

static bool thread_running = false;
static HANDLE ht_proclog;

void callback_commit_file(const CMOVE_LOG_CONTEXT *lc, std::ostream &dllcerr)
{
    if(lc == NULL)
    {
        thread_running = false;
        return;
    }
    //save_file();
    if(strcmp(lc->file.patientID, lc->patient.patientID) == 0)
    {   //refresh_patient_info();
        dllcerr << "refresh patient info" << endl;
    }
}

static void thread_wrapper(void *pSessionId)
{
    process_log(static_cast<string*>(pSessionId)->c_str(), true, callback_commit_file);
}

void call_process_log(std::string &sessionId)
{
    thread_running = true;
    ht_proclog = (HANDLE)_beginthread(thread_wrapper, 0, static_cast<void*>(&sessionId));
    if(ht_proclog == INVALID_HANDLE_VALUE)
    {
        cerr << "can't create thread to process log" << endl;
        thread_running = false;
        return;
    }
    while(thread_running)
    {
        cerr << "############################################################" << endl;
        Sleep(10);
    }
}
