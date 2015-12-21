#include "stdafx.h"

using namespace std;
    
static ASSOC_CONTEXT assoc;

static int cmd_assoc(char type, istringstream &cmdstrm)
{
    unsigned int tag = 0;
    cmdstrm >> hex >> tag;
    cerr << type << " " << hex << uppercase << setw(8) << setfill('0') << tag;
    switch(tag)
    {
    case ASSOC_ESTA:
        cmdstrm >> assoc.callingAE >> assoc.callingAddr >> assoc.calledAE >> dec >> assoc.port >> assoc.calledAddr;
        cerr << " " << assoc.callingAE << " " << assoc.callingAddr<< " " << assoc.calledAE<< " " << dec << assoc.port << " " << assoc.calledAddr << endl;
        break;
    case ASSOC_TERM:
    case ASSOC_ABORT:
        cerr << endl;
        return 0;
    default:
        char otherbuf[1024] = "";
        cmdstrm.getline(otherbuf, sizeof(otherbuf));
        cerr << otherbuf << ": can't process" << endl;
        break;
    }
    return 1;
}

static int process_cmd(const char *buf, size_t buf_len)
{
    char type = '\0';
    
    istringstream cmdstrm(buf);
    cmdstrm >> type;

    switch(type)
    {
    case TYPE_ASSOC:
        return cmd_assoc(type, cmdstrm);
    case TYPE_FILE:
        cerr << buf << endl;
        break;
    case TYPE_PATIENT:
        cerr << buf << endl;
        break;
    case TYPE_STUDY:
        cerr << buf << endl;
        break;
    case TYPE_SERIES:
        cerr << buf << endl;
        break;
    case TYPE_INSTANCE:
        cerr << buf << endl;
        break;
    default:
        cerr << "can't recognize command: " << buf << endl;
    }
    return 1;
}

static char buff[1024];
void process_log(const string &sessionId)
{
    size_t gpos = 0;
    string fn(sessionId);
    fn.append(1, '\\').append("cmove.txt");
    ifstream tail(fn, ios_base::in, _SH_DENYNO);
    if(tail.fail())
    {
        cerr << "无法打开文件" << fn << endl;
        return;
    }
    int ret = 1, waitTime = 0;
    assoc.port = 0;
    while(ret && waitTime <= 10 * 1000)
    {
        tail.getline(buff + gpos, sizeof(buff) - gpos);
        streamsize gcnt = tail.gcount();
        if(gcnt > 0)
        {
            waitTime = 0;
            gpos += static_cast<size_t>(gcnt);
            if(!tail.fail()) 
            {
                ret = process_cmd(buff, gpos);
                gpos = 0;
            }
        }
        if(tail.fail() || tail.eof())
        {
            if(tail.fail()) cerr << " fail";
            if(tail.eof()) cerr << " eof";
            cerr << endl;
            tail.clear();
            waitTime += 100;
            Sleep(100);
        }
    }
    tail.close();
}
