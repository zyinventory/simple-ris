#include "stdafx.h"
#include "commonlib.h"

using namespace std;

extern bool opt_verbose;
extern DWORD sys_core_num;
CRITICAL_SECTION  cs_queue;

bool CMOVE_FILE_SECTION::StorePath(char *buf, size_t buf_size)
{
    if(buf_size < 63) return false;
    uidHash(studyUID, buf, buf_size);
    buf[8] = '\\';
    SeriesInstancePath(seriesUID, instanceUID, buf + 9, buf_size - 9);
    return true;
}

static deque<CMOVE_LOG_CONTEXT> file_queue;

size_t file_queue_length()
{
    size_t ql = 0;
    BOOL te = TryEnterCriticalSection(&cs_queue);
    ql = file_queue.size();
    if(te) LeaveCriticalSection(&cs_queue);
    return ql;
}

bool commit_file_to_queue(CMOVE_LOG_CONTEXT &lc)
{
    if(TryEnterCriticalSection(&cs_queue))
    {
        file_queue.push_back(lc);
        LeaveCriticalSection(&cs_queue);
        cerr << "commit " << lc.file.filename << endl;
        return true;
    }
    else
    {
        cerr << "can't commit " << lc.file.filename << ", timeout" << endl;
        return false;
    }
}

extern volatile bool end_of_input_queue;

void thread_read_queue(void *p)
{
    CMOVE_LOG_CONTEXT lc;

    while(true)
    {
        bool get_lc = false;
        if(TryEnterCriticalSection(&cs_queue))
        {
            if(file_queue.empty())
            {
                bool ready_exit = end_of_input_queue;
                LeaveCriticalSection(&cs_queue);
                if(ready_exit) break;
            }
            else
            {
                lc = file_queue.front();
                file_queue.pop_front();
                LeaveCriticalSection(&cs_queue);
                get_lc = true;
            }
        }

        if(get_lc) // && waiting process_queue is not full
        {
            // start process
            char storePath[64];
            lc.file.StorePath(storePath, sizeof(storePath));
            cerr << storePath << endl;
            cerr << lc.file.filename << endl;
            cerr << lc.file.patientID << endl;
            cerr << lc.file.studyUID << endl;
            cerr << lc.file.seriesUID << endl;
            cerr << lc.file.instanceUID << endl;
            cerr << lc.file.xfer << " " << lc.file.isEncapsulated << endl;
        }

        if(end_of_input_queue) continue;
        else if(!get_lc) { Sleep(10); continue; }
    }
    cerr << "queue is empty" << endl;
}
