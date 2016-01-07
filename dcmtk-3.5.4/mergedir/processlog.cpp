#include "stdafx.h"
using namespace std;

static int start_write_log = 0;
static bool inFile = false;

static void close_cmdfile(ofstream &cmdfile)
{
    cmdfile.close();
    inFile = false;
    ++start_write_log;
    Sleep(rand() % 30);
}

static void test_sim_slow_log_writer(void *seid)
{
    char src_name[MAX_PATH];
    sprintf_s(src_name, "%s\\cmove.txt", (const char*)seid);

    ifstream strmlog(src_name, ios_base::in, _SH_DENYNO);
    if(strmlog.fail())
    {
        start_write_log = -1;
        displayErrorToCerr("test_sim_slow_log_writer()", GetLastError());
        return;
    }
    sprintf_s(src_name, "%s\\%s\\log\\", _getcwd(NULL, 0), seid);
    char buf[1024];
    int fnpos = 0;
    ofstream cmdfile;
    do {
        char fn[MAX_PATH];
        strmlog.getline(buf, sizeof(buf));
        if(strmlog.eof()) break;
        switch(buf[0])
        {
        case 'M':
        case 'T':
            fnpos = GetNextUniqueNo(src_name, fn, sizeof(fn));
            sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
            if(inFile) close_cmdfile(cmdfile);
            cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
            inFile = !cmdfile.fail();
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
            }
            else cerr << "can't write " << buf << " to file " << fn << endl;
            break;
        case 'F': // F tag must be coupled !!!
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
            }
            else
            {
                int fnpos = GetNextUniqueNo(src_name, fn, sizeof(fn));
                sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
                if(inFile) close_cmdfile(cmdfile);
                cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
                inFile = !cmdfile.fail();
                if(inFile) cmdfile << buf << endl;
                else cerr << "can't write " << buf << " to file " << fn << endl;
            }
            break;
        default:
            if(inFile) cmdfile << buf << endl;
            else cerr << "can't write " << buf << " to file " << fn << endl;
        }
    } while(!strmlog.eof());
    if(inFile) cmdfile.close();
    strmlog.close();
}
/*
static void test_consume_log(const char *sid)
{
    char log_name[MAX_PATH];
    sprintf_s(log_name, "%s\\cmove.txt", sid);

    ifstream tail(log_name, ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << log_name << endl;
        return;
    }

    while(true)
    {
        if(const char *buff = try_read_line(tail))
        {
            cout << buff << endl;
            if(buff[0] == 'M') break;
        }
        else
            Sleep(1);
    }
    tail.close();
}
*/
void call_process_log(std::string &sessionId)
{
    char src_name[MAX_PATH];
    sprintf_s(src_name, "%s\\%s\\log", _getcwd(NULL, 0), sessionId.c_str());
    if(_mkdir(src_name) && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir log faile: " << msg << endl;
        return;
    }
    HANDLE ht = (HANDLE)_beginthread(test_sim_slow_log_writer, 0, (void*)sessionId.c_str());
    //test_sim_slow_log_writer((void*)sessionId.c_str());
    int i = 10;
    while(start_write_log == 0 && i < 1000)
    {
        Sleep(i);
        i += 10;
    }
    if(start_write_log > 0)
    {
        scp_store_main_loop(sessionId.c_str(), false);
        //test_consume_log(sessionId.c_str());
    }
    WaitForSingleObject(ht, INFINITE);
}

void clear_resource()
{
    ReleaseUniqueNoResource();
}
