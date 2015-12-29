#include "stdafx.h"
using namespace std;

bool start_write_log =false;

static void test_sim_slow_log_writer(void *seid)
{
    char src_name[MAX_PATH], log_name[MAX_PATH];
    sprintf_s(src_name, "%s\\cmove.src.txt", (const char*)seid);
    sprintf_s(log_name, "%s\\cmove.txt", (const char*)seid);

    FILE *fplog =fopen(log_name, "w+b"), *fpsrc = fopen(src_name, "rb");
    if(fpsrc == NULL)
    {
        if(fplog != NULL) fclose(fplog);
        start_write_log = true;
        return;
    }
    else if(fplog == NULL)
    {
        fclose(fpsrc);
        start_write_log = true;
        return;
    }
    char buf[16];
    size_t readNum = 0;
    start_write_log = true;
    do {
        readNum = fread(buf, 1, sizeof(buf), fpsrc);
        fwrite(buf, 1, readNum, fplog);
        fflush(fplog);
        Sleep(rand() % 30);
    } while(readNum);
    fclose(fplog);
    fclose(fpsrc);
}

static char buff[1024];
static size_t gpos = 0;

static bool try_read_line(ifstream &tail)
{
    tail.getline(buff + gpos, sizeof(buff) - gpos);
    streamsize gcnt = tail.gcount();
    if(gcnt > 0) gpos += static_cast<size_t>(gcnt);
    if(tail.fail() || tail.eof())
    {
        cerr << "read " << setw(2) << setfill(' ') << gcnt << " bytes";
        if(tail.eof()) cerr << ", encouter eof";
        cerr << endl;
        tail.clear();
        return false;
    }
    else
        return true;
}

void call_process_log(std::string &sessionId)
{
    //test_sim_slow_log_writer((void*)sessionId.c_str());
    _beginthread(test_sim_slow_log_writer, 0, (void*)sessionId.c_str());
    while(!start_write_log) Sleep(10);
    process_log(sessionId.c_str(), false);
    /*
    char log_name[MAX_PATH];
    sprintf_s(log_name, "%s\\cmove.txt", sessionId.c_str());

    ifstream tail(log_name, ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << log_name << endl;
        return;
    }

    while(true)
    {
        if(try_read_line(tail))
        {
            cout << buff << endl;
            gpos = 0;
            if(strcmp("T FFFFFFFF", buff) == 0) break;
        }
        else
        {
            Sleep(1);
        }
    }
    tail.close();
    */
}
