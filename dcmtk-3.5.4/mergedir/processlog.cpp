#include "stdafx.h"
using namespace std;

void callback_commit_file(const CMOVE_LOG_CONTEXT *lc)
{
    //save_file();
    if(strcmp(lc->file.patientID, lc->patient.patientID) == 0)
    {   //refresh_patient_info();
        cerr << "refresh patient info" << endl;
    }
}

void call_process_log(const std::string &sessionId)
{
    return process_log(sessionId.c_str(), true, callback_commit_file);
}
