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
    WIN32_FIND_DATA wfd;
    char fileFilter[MAX_PATH] = "state.move\\*.dfc";

    HANDLE hDiskSearch = FindFirstFile(fileFilter, &wfd);
    if(hDiskSearch == INVALID_HANDLE_VALUE)
    {
        displayErrorToCerr("test_sim_slow_log_writer()", GetLastError());
        return;
    }
    list<string> dfc_files;
    do
	{
        string dfc(wfd.cFileName);
        if (dfc.compare(".") == 0 || dfc.compare("..") == 0) 
			continue; // skip . ..
        if(0 == (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            dfc_files.push_back(dfc);
	} while (FindNextFile(hDiskSearch, &wfd));
	FindClose(hDiskSearch); // ¹Ø±Õ²éÕÒ¾ä±ú
    
    // load dll into memory
    in_process_sequence_dll(fileFilter, sizeof(fileFilter), "state\\");

    dfc_files.sort();
    LARGE_INTEGER last_time = { 0, 0};
    char *buff = new char[4096];
    int i = 0;
    for(list<string>::iterator it = dfc_files.begin(); it != dfc_files.end(); ++it, ++i)
    {
        size_t pos1 = 0;
        size_t pos2 = it->find('.');
        if(pos2 == string::npos) {
            cerr << "invalid dfc file name: " << *it << endl; continue;
        }
        string integer_sec_str(it->substr(pos1, pos2 - pos1));
        if(pos2 == string::npos) { 
            cerr << "invalid dfc file name: " << *it << endl; continue;
        }
        
        pos1 = pos2 + 1;
        pos2 = it->find('-', pos1);
        if(pos2 == string::npos) {
            cerr << "invalid dfc file name: " << *it << endl;
            continue;
        }
        string integer_ms_str(it->substr(pos1, pos2 - pos1));

        char *pstop = NULL;
        LARGE_INTEGER sec_number;
        sec_number.QuadPart = _atoi64(integer_sec_str.c_str());
        if(pstop == integer_sec_str.c_str()) {
            cerr << "invalid sec number: " << integer_sec_str << endl;
            continue;
        }

        struct tm tstr;
        tstr.tm_sec = sec_number.QuadPart % 100;
        sec_number.QuadPart /= 100;
        tstr.tm_min = sec_number.QuadPart % 100;
        sec_number.QuadPart /= 100;
        tstr.tm_hour = sec_number.QuadPart % 100;
        sec_number.QuadPart /= 100;
        tstr.tm_mday = sec_number.QuadPart % 100;
        sec_number.QuadPart /= 100;
        tstr.tm_mon = sec_number.QuadPart % 100 - 1;
        sec_number.QuadPart /= 100;
        tstr.tm_year = sec_number.QuadPart - 1900;
        __time64_t tsec = _mktime64(&tstr);

        LARGE_INTEGER ms_number;
        ms_number.QuadPart = _atoi64(integer_ms_str.c_str());
        if(pstop == integer_ms_str.c_str()) {
            cerr << "invalid ms number: " << integer_ms_str << endl;
            continue;
        }
        ms_number.QuadPart += tsec * 1000;

        pos1 = pos2 + 1;
        pos2 = it->find('-', pos1);
        if(pos2 == string::npos) {
            cerr << "invalid dfc file name: " << *it << endl;
            continue;
        }
        string diff_number(it->substr(pos1, pos2 - pos1).c_str());
        LARGE_INTEGER diff;
        diff.QuadPart = _strtoi64(diff_number.c_str(), &pstop, 10);
        if(pstop == diff_number.c_str()) {
            cerr << "invalid diff number: " << diff_number << endl;
            continue;
        }

        ms_number.QuadPart -= diff.QuadPart;

        if(last_time.QuadPart) diff.QuadPart = ms_number.QuadPart - last_time.QuadPart;
        else diff.QuadPart = 0LL;

        last_time = ms_number;

        if(diff.LowPart > 10 * 1000)
            cerr << "wait too long: " << diff.LowPart << endl;
        else
        {
            if(diff.LowPart > 10)
                Sleep(diff.LowPart - 10);
            else
                Sleep(0);
        }
        string ipath("state.move\\");
        
        ipath.append(*it);
        ifstream idfc(ipath);
        if(idfc.fail()) {
            cerr << "can't open src dfc: " << ipath << endl; continue;
        }
        ostringstream ostrm;
        while(!idfc.fail())
        {
            size_t rlen = idfc.read(buff, 4096).gcount();
            ostrm.write(buff, rlen);
        }
        idfc.close();

        string content = ostrm.str();

        char newfn_buff[MAX_PATH];
        size_t used = in_process_sequence_dll(newfn_buff, sizeof(newfn_buff), "state\\");
        string::size_type postfix = it->find_last_of('_');
        strcpy_s(newfn_buff + used, sizeof(newfn_buff) - used, it->substr(postfix).c_str());

        ofstream odfc(newfn_buff, ios_base::out | ios_base::trunc);
        if(odfc.fail()) {
            cerr << "can't open dest dfc: " << newfn_buff << endl;
            continue;
        }
        odfc.write(content.c_str(), content.length());
        odfc.close();
    }
    if(buff) delete[] buff;
}

void call_process_log(const std::string &storedir, const std::string &sessionId, bool verbose)
{
    char src_name[MAX_PATH];
    sprintf_s(src_name, "%s\\%s", storedir.c_str(), sessionId.c_str());
    if(_chdir(src_name))
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "call_process_log() chdir to " << src_name << " failed: " << msg << endl;
        return;
    }
    if(_mkdir("state") && errno != EEXIST)
    {
        char msg[1024];
        strerror_s(msg, errno);
        cerr << "call_process_log() mkdir state faile: " << msg << endl;
        return;
    }

    //HANDLE ht = (HANDLE)_beginthread(test_sim_slow_log_writer, 0, NULL);
    //test_sim_slow_log_writer(NULL);
    if(start_write_log >= 0) // start_write_log == 0, start immediately
    {
        //test_consume_log(sessionId.c_str());
        scp_store_main_loop(sessionId.c_str(), verbose);
        //test_for_make_index("C:\\usr\\local\\dicom", true);
    }
    //WaitForSingleObject(ht, INFINITE);
}

void clear_resource()
{
    ReleaseUniqueNoResource();
}
