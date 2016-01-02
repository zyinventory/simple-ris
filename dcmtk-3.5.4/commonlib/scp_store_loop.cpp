#include "stdafx.h"
#include "commonlib.h"
#include "commonlib_internal.h"

using namespace std;

bool opt_verbose = false;
char pacs_base[MAX_PATH];

static const char *sessionId;
static char buff[1024];
static size_t gpos = 0;

COMMONLIB_API char *try_read_line(ifstream &tail)
{
    tail.getline(buff + gpos, sizeof(buff) - gpos);
    streamsize gcnt = tail.gcount();
    if(gcnt > 0) gpos += static_cast<size_t>(gcnt);
    if(tail.fail() || tail.eof())
    {
        /* if(opt_verbose) */cerr << "<";
        tail.clear();
        return NULL;
    }
    else
    {
        gpos = 0;
        return buff;
    }
}

bool run_index();

COMMONLIB_API void scp_store_main_loop(const char *sessId, bool verbose)
{
    
    opt_verbose = verbose;
    sessionId = sessId;
    string fn("\\storedir\\");
    fn.append(sessionId);
    if(ChangeToPacsWebSub(pacs_base, MAX_PATH, fn.c_str()))
    {
        cerr << "无法切换工作目录" << endl;
        return;
    }
    ifstream tail("cmove.txt", ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << fn << endl;
        return;
    }

    int ret = 1, waitTime = 0;
    clear_log_context();

    while(ret && waitTime <= 10 * 1000)
    {
        if(const char * line = try_read_line(tail))
        {
            waitTime = 0;
            ret = process_cmd(line);
        }
        else
        {
            waitTime += 100;
            Sleep(100);
        }
    }
    tail.close();
    while(commit_file_to_workers(NULL)) Sleep(100);
    while(run_index());
}
