// jobloader.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

using namespace std;

static ofstream flog;
int lock_number = -1, *ptr_license_count = NULL;
char lock_file_name[64] = "..\\etc\\*.key", buff[1024];

int _tmain(int argc, _TCHAR* argv[])
{
	int ret = 0;
    bool debug_mode = false, balance = true;
    HANDLE hParentProcess = NULL, hmap = NULL, hfile = INVALID_HANDLE_VALUE, hdir = INVALID_HANDLE_VALUE;
    HANDLE wa[2];
    list<string> pending_jdf;

	if(ChangeToPacsWebSub(NULL, 0)) return -3;

	GenerateTime("pacs_log\\%Y\\%m\\%d\\%H%M%S_jobloader.txt", buff, sizeof(buff));
	if(PrepareFileDir(buff))
    {
        flog.open(buff);
        if(flog.fail())
        {
            cerr << "JobLoader open " << buff << " failed" << endl;
            return -4;
        }
    }
	else return -4;

    if(LoadSettings("..\\etc\\settings.ini", flog, true))
    {
        if(GetSetting("JobloaderDebugMode", buff, sizeof(buff)))
        {
            if(strcmp("1", buff) == 0) debug_mode = true;
        }
        if(GetSetting("JobloaderBalance", buff, sizeof(buff)))
        {
            if(strcmp("0", buff) == 0) balance = false;
        }
    }
    else time_header_out(flog) << "Load Settings failed." << endl;
    
    if(debug_mode) time_header_out(flog) << "enable debug mode." << endl;

    size_t required = 0;
    if(getenv_s(&required, buff, "PARENT_PID"))
    {
        displayErrorToCerr("JobLoader get parent pid", GetLastError(), &flog);
        ret = -2;
        goto exit_job_loader;
    }
    else
    {
        size_t pid = atoi(buff);
        if(pid) hParentProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if(hParentProcess == NULL)
        {
            displayErrorToCerr("JobLoader get parent process handle", GetLastError(), &flog);
            ret = -2;
            goto exit_job_loader;
        }
    }
    if(debug_mode) time_header_out(flog) << "open parent pid OK: " << buff << endl;

    sprintf_s(buff, "%s\\orders_balance", GetPacsBase());
    hdir = FindFirstChangeNotification(buff, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    if(hdir == INVALID_HANDLE_VALUE)
    {
        ret = GetLastError();
        sprintf_s(buff, "FindFirstChangeNotification(%s)", buff);
        displayErrorToCerr(buff, ret, &flog);
        goto exit_job_loader;
    }
    if(debug_mode) time_header_out(flog) << "dir monitor OK: " << buff << endl;

    ptr_license_count = reinterpret_cast<int*>(create_shared_memory_mapping("job_loader_lock_counter", sizeof(int), &hmap, &hfile, &flog));

    char rw_passwd[9] = "";
#ifdef NDEBUG
    int license_count = -1;
	if(InitiateLock(0))
    {
		lock_number = getLockNumber(lock_file_name, FALSE, lock_file_name + 7, sizeof(lock_file_name) - 7);
        SEED_SIV siv;
        if(lock_number != -1 && 0 == loadPublicKeyContentRW(lock_file_name, &siv, lock_number, rw_passwd))
		{
            if(!invalidLock("..\\etc\\license.key", lock_file_name, &siv))
			{
                license_count = currentCount(rw_passwd);
                if(ptr_license_count) *ptr_license_count = license_count;
                time_header_out(flog) << "lock counter is " << license_count << endl;
            }
            else
            {
                time_header_out(flog) << "invalidLock() failed:" << lock_number << endl;
                goto exit_job_loader;
            }
        }
        else
        {
            time_header_out(flog) << "get lock number failed:" << lock_number << endl;
            goto exit_job_loader;
        }
    }
	else
    {
        time_header_out(flog) << "init lock failed:" << hex << LYFGetLastErr() << endl;
        goto exit_job_loader;
    }
#else
    int license_count = 101;
#endif
    if(ptr_license_count) *ptr_license_count = license_count;

    wa[0] = hdir;
    wa[1] = hParentProcess;
    bool first_boot = true;
    while(true)
    {
        DWORD wr = WAIT_TIMEOUT;
        if(first_boot)
        {
            wr = WAIT_OBJECT_0;
            first_boot = false;
        }
        else wr = WaitForMultipleObjects(2, wa, FALSE, 1000);

        if(wr == WAIT_OBJECT_0 || wr == WAIT_ABANDONED_0)
        {   // orders_balance dir change
            if(debug_mode) time_header_out(flog) << "orders_balance dir is changed." << endl;

            pending_jdf.clear();
            WIN32_FIND_DATA wfd;
            HANDLE h_jdf_find = FindFirstFile("..\\orders_balance\\*.jdf", &wfd);
            if(h_jdf_find != INVALID_HANDLE_VALUE)
            {
                do {
                    if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                    pending_jdf.push_back(wfd.cFileName);
                    if(debug_mode) time_header_out(flog) << "jdf found: " << wfd.cFileName << endl;
                } while(FindNextFile(h_jdf_find, &wfd));
                FindClose(h_jdf_find);
                pending_jdf.sort();
            }

            bool no_license = false, no_valid_publish = false;
            while(pending_jdf.size())
            {
                int try_publish_ret = 0;
#ifdef NDEBUG
                license_count = currentCount(rw_passwd);
                license_count = (license_count < 0 ? 0 : license_count);
                if(ptr_license_count) *ptr_license_count = license_count;
                if(license_count > 0)
				{
                    if(no_license)
                    {
                        time_header_out(flog) << "new license is comming: " << license_count << endl;
                        no_license = false;
                    }
#endif
                    list<string>::iterator it = pending_jdf.begin();
                    string jdf_file(*it);
                    
                    if(debug_mode && !no_valid_publish) time_header_out(flog) << "TryPublishJDF(" << *it << ")." << endl;

                    ClearGenerateIndexLog();
                    if(balance) // load balance to epson
                        try_publish_ret = TryPublishJDF(debug_mode, jdf_file.c_str());
                    else
                    {   // remove jdf only
                        string file_removed("..\\orders_balance\\");
                        file_removed.append(jdf_file);
                        if(_unlink(file_removed.c_str()))
                        {
                            int en = errno;
                            char msg[256];
                            try_publish_ret = TryPublishJDF_SrcOpenError;
                            strerror_s(msg, en);
                            time_header_out(flog) << "remove " << file_removed << " failed:" << msg << endl;
                        }
                        else try_publish_ret = TryPublishJDF_PublishOK;
                    }
                    if(try_publish_ret == TryPublishJDF_PublishOK) // debug_mode is controlled by JobloaderDebugMode in settings.ini
                    {
#ifdef NDEBUG
                        license_count = decreaseCount(rw_passwd);
                        if(ptr_license_count) *ptr_license_count = (license_count < 0 ? 0 : license_count);
#endif
                        time_header_out(flog) << "publish " << jdf_file << " OK." << endl;
                        pending_jdf.erase(it);
                    }
                    else if(try_publish_ret < 0) // TryPublishJDF_SrcOpenError || TryPublishJDF_SrcMarkError
                    {
                        time_header_out(flog) << "publish " << jdf_file << " failed, remove it." << endl;
                        pending_jdf.erase(it);
                    }
                    //else TryPublishJDF_PublisherBusy || TryPublishJDF_NoValidPrinter, wait for 5 sec
                    
                    if(debug_mode)
                    {
                        if(try_publish_ret != TryPublishJDF_NoValidPrinter || !no_valid_publish)
                        {
                            const char *msg = GetGenerateIndexLog();
                            if(msg) time_header_out(flog) << "TryPublishJDF(" << jdf_file << ") return " 
                                << try_publish_ret << ", log message:" << endl << msg << endl;
                        }
                    }
                    no_valid_publish = (try_publish_ret == TryPublishJDF_NoValidPrinter);
                    ClearGenerateIndexLog();
#ifdef NDEBUG
                }
                else
                {
                    if(!no_license)
                    {
                        time_header_out(flog) << "lock counter is 0, waiting new license..." << endl;
                        no_license = true;
                    }
                }
#endif
                if(try_publish_ret >= 0) // busy, no license or publish OK, wait 5 sec
                {
                    wr = WaitForSingleObject(hParentProcess, 5000);
                    if(wr != WAIT_TIMEOUT)
                    {
                        if(wr == WAIT_FAILED)
                            ret = displayErrorToCerr("WaitForSingleObject(ParentProcHandle)", GetLastError(), &flog);
                        else
                            time_header_out(flog) << "parent process exit, proc handle is released(WaitForSingleObject)." << endl;
                        goto exit_job_loader;
                    }
                }
            } // while(pending_jdf.size()), TryPublishJDF() loop

            // no pending file or parent proc exit
            if(! FindNextChangeNotification(hdir))
            {
                ret = displayErrorToCerr("FindNextChangeNotification()", GetLastError(), &flog);
                break; // break from main loop
            }
        } // file change in orders_balance
        else if(wr == WAIT_OBJECT_0 + 1 || wr == WAIT_ABANDONED_0 + 1)
        {
            time_header_out(flog) << "parent process exit, proc handle is released." << endl;
            break;
        }
        else if(wr == WAIT_FAILED)
        {
            ret = displayErrorToCerr("WaitForMultipleObjects()", GetLastError(), &flog);
            break; // break from main loop
        }
        //else // wr == WAIT_TIMEOUT
    } // while(true), exit when parent proc exit or error

exit_job_loader:
    close_shared_mapping(ptr_license_count, hmap, hfile);
    if(hdir != INVALID_HANDLE_VALUE) { CloseHandle(hdir); }
    if(hParentProcess) CloseHandle(hParentProcess);
    if(flog.is_open()) flog.close();
    return 0;
}
