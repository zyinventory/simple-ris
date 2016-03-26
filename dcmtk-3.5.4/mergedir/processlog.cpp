#include "stdafx.h"
#import <msxml3.dll>

using namespace std;

static int start_write_log = 0;
static bool inFile = false;

static void close_cmdfile(ofstream &cmdfile)
{
    cmdfile.close();
    inFile = false;
    ++start_write_log;
    Sleep(10 + rand() % 50);
}

static void test_sim_slow_log_writer(void*)
{
    ifstream strmlog("cmove.txt", ios_base::in, _SH_DENYNO);
    if(strmlog.fail())
    {
        start_write_log = -1;
        displayErrorToCerr("test_sim_slow_log_writer()", GetLastError());
        return;
    }
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
            fnpos = GetNextUniqueNo("state\\", fn, sizeof(fn));
            sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
            if(inFile) close_cmdfile(cmdfile);
            cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
            inFile = !cmdfile.fail();
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
            }
            else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
            break;
        case 'F': // F tag must be coupled !!!
        case 'N': // N tag must be coupled !!!
            if(inFile)
            {
                cmdfile << buf << endl;
                close_cmdfile(cmdfile);
                inFile = false;
            }
            else
            {
                int fnpos = GetNextUniqueNo("state\\", fn, sizeof(fn));
                sprintf_s(fn + fnpos, sizeof(fn) - fnpos, "_%c.dfc", buf[0]);
                if(inFile) close_cmdfile(cmdfile);
                cmdfile.open(fn, ios_base::out | ios_base::trunc, _SH_DENYRW);
                inFile = !cmdfile.fail();
                if(inFile) cmdfile << buf << endl;
                else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
            }
            break;
        default:
            if(inFile) cmdfile << buf << endl;
            else cerr << "can't understand how to write " << buf << " to file " << fn << endl;
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

template<class Pred> static void find_recursive(const string &input_path, list<string> &collector, Pred pred)
{
    string path(input_path);
    if(path.empty()) path = ".";
    string::size_type old_size = path.size();
    string::iterator it = path.end();
    --it;
    if(*it != '\\') path.append(1, '\\');
    path.append(1, '*');
    it = path.end();
    --it; // it -> '*'

    struct __finddata64_t wfd;
    intptr_t hSearch = _findfirst64(path.c_str(), &wfd);
    if(hSearch == -1)
    {
        perror("find_recursive() failed");
        fprintf_s(stderr, "\tfind %s failed\n", path.c_str());
        return;
    }
    path.erase(it);
    do {
        if(wfd.attrib & _A_SUBDIR)
        {
            if(strcmp(wfd.name, ".") == 0 || strcmp(wfd.name, "..") == 0) continue;
            size_t old_len = path.length();
            path.append(wfd.name);
            find_recursive(path, collector, pred);
            path.resize(old_len);
        }
        else  // file
        {
            if(pred(&wfd))
            {
                size_t old_len = path.length();
                path.append(wfd.name);
                collector.push_back(path);
                path.resize(old_len);
            }
        }
	} while(_findnext64(hSearch, &wfd) == 0);
	_findclose(hSearch);
}

static bool is_ext_file(const struct __finddata64_t *wfd, const char *ext)
{
    if((wfd->attrib & _A_SUBDIR) == 0)
    {
        const char * p = strstr(wfd->name, ext);
        if(p && strcmp(p, ext) == 0) return true;
    }
    return false;
}

static void find_recursive_xml_file(string &input_path, list<string> &collector)
{
    find_recursive(input_path, collector, bind2nd(ptr_fun(is_ext_file), ".xml"));
}

void move_index_receive(const char *pacs_base, map<string, bool> &map_receive_index)
{
    list<string> collector;
    string path("indexdir\\receive");
    //find_recursive(path, collector, [](const struct __finddata64_t *wfd) { return is_ext_file(wfd, ".xml"); });
    find_recursive_xml_file(path, collector);

    for(list<string>::iterator it = collector.begin(); it != collector.end(); ++it)
    {
        bool move_ok = false;
        char dest_path[MAX_PATH];
        sprintf_s(dest_path, "%s\\pacs\\%s", pacs_base, it->c_str());
        if(PrepareFileDir(dest_path))
        {
            if(_unlink(dest_path) == 0 || errno == ENOENT)
            {
                if(rename(it->c_str(), dest_path))
                {
                    char msg[1024];
                    strerror_s(msg, errno);
                    cerr << "move_index_receive() move " << *it << " to " << dest_path << " failed: " << msg << endl;
                }
                else
                {
                    move_ok = true;
                    cout << "trigger index_receive " << dest_path << endl;
                }
            }
            else
            {
                char msg[1024];
                strerror_s(msg, errno);
                cerr << "move_index_receive() delete " << dest_path << " failed: " << msg << endl;
            }
        }
        else
            cerr << "move_index_receive() can't mkdir " << dest_path << endl;
        map_receive_index[*it] = move_ok;
    }
}

void call_process_log(const std::string &storedir, const std::string &sessionId)
{
    char src_name[MAX_PATH];
    sprintf_s(src_name, "%s\\%s", storedir.c_str(), sessionId.c_str());
    if(_chdir(src_name))
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "chdir to " << src_name << " failed: " << msg << endl;
        return;
    }
    if(_mkdir("state") && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "mkdir state faile: " << msg << endl;
        return;
    }
    HANDLE ht = (HANDLE)_beginthread(test_sim_slow_log_writer, 0, NULL);
    //test_sim_slow_log_writer((void*)sessionId.c_str());
    /*
    int i = 10;
    while(start_write_log == 0 && i < 1000)
    {
        Sleep(i);
        i += 10;
    }
    */
    if(start_write_log >= 0) // start_write_log == 0, start immediately
    {
        scp_store_main_loop(sessionId.c_str(), false);
        //test_consume_log(sessionId.c_str());

        map<string, bool> map_receive_index;
        //move_index_receive(pacs_base, map_receive_index);

        // todo: add study xml to patient xml and study date xml
    }
}

void clear_resource()
{
    ReleaseUniqueNoResource();
}
