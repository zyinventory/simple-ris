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
    bool debug_mode = false;
    HANDLE hMutex = NULL, hServiceMutex = NULL, hmap = NULL, hfile = INVALID_HANDLE_VALUE, hdir = INVALID_HANDLE_VALUE;
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
        if(GetSetting("DebugMode", buff, sizeof(buff)))
        {
            if(strcmp("1", buff) == 0) debug_mode = true;
        }
    }
    else time_header_out(flog) << "Load Settings failed." << endl;
    
    if(debug_mode) time_header_out(flog) << "enable debug mode." << endl;

    hMutex = CreateMutex(NULL, TRUE, "Global\\publisher_job_load_balance_mutex");
    if(hMutex == NULL)
    {
        displayErrorToCerr("JobLoader CreateMutex() self", GetLastError(), &flog);
		ret = -1;
        goto exit_job_loader;
    }
    if(debug_mode) time_header_out(flog) << "create mutex OK: Global\\publisher_job_load_balance_mutex." << endl;

    if(argc > 1)
    {
        sprintf_s(buff, "Global\\%s", argv[1]);
        hServiceMutex = OpenMutex(SYNCHRONIZE, FALSE, buff);
        if(hServiceMutex == NULL)
        {
            strcat_s(buff, " JobLoader CreateMutex() parent");
            displayErrorToCerr(buff, GetLastError(), &flog);
            ret = -2;
            goto exit_job_loader;
        }
        if(debug_mode) time_header_out(flog) << "open mutex OK: Global\\" << argv[1] << endl;
    }

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
    wa[1] = hServiceMutex;
    bool first_boot = true;
    while(true)
    {
        DWORD wr = WAIT_TIMEOUT;
        if(first_boot)
        {
            wr = WAIT_OBJECT_0;
            first_boot = false;
        }
        else wr = WaitForMultipleObjects(hServiceMutex ? 2 : 1, wa, FALSE, 1000);

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

            while(pending_jdf.size())
            {
                int try_publish_ret = 0;
#ifdef NDEBUG
				if(currentCount(rw_passwd) > 0)
				{
#endif
                    list<string>::iterator it = pending_jdf.begin();
                    string jdf_file(*it);
                    
                    if(debug_mode) time_header_out(flog) << "TryPublishJDF(" << *it << ")." << endl;

                    ClearGenerateIndexLog();
                    try_publish_ret = TryPublishJDF(debug_mode, it->c_str());
                    if(try_publish_ret == TryPublishJDF_PublishOK) // debug_mode is controlled by DebugMode in settings.ini
                    {
#ifdef NDEBUG
                        license_count = decreaseCount(rw_passwd);
                        if(ptr_license_count) *ptr_license_count = license_count;
#endif
                        time_header_out(flog) << "publish " << jdf_file << " OK." << endl;
                        pending_jdf.erase(it);
                    }
                    else if(try_publish_ret < 0) // TryPublishJDF_SrcOpenError || TryPublishJDF_SrcMarkError
                    {
                        time_header_out(flog) << "publish " << jdf_file << " failed, remove it." << endl;
                        pending_jdf.erase(it);
                    }
                    //else TryPublishJDF_PublisherBusy, wait for 5 sec

                    if(debug_mode)
                    {
                        const char *msg = GetGenerateIndexLog();
                        if(msg) time_header_out(flog) << "TryPublishJDF(" << jdf_file << ") return " 
                            << try_publish_ret << ", log message:" << endl << msg << endl;
                    }
                    ClearGenerateIndexLog();
#ifdef NDEBUG
                }
                else time_header_out(flog) << "lock counter is 0." << endl;
#endif
                if(try_publish_ret >= 0) // busy or publish OK, wait 5 sec
                {
                    if(hServiceMutex)
                    {
                        wr = WaitForSingleObject(hServiceMutex, 5000);
                        if(wr != WAIT_TIMEOUT)
                        {
                            if(wr == WAIT_FAILED)
                                ret = displayErrorToCerr("WaitForSingleObject(ParentProcHandle)", GetLastError(), &flog);
                            else
                                time_header_out(flog) << "parent process exit, mutex is released(WaitForSingleObject)." << endl;
                        }
                    }
                    else Sleep(5000); // no parent proc mutex
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
            time_header_out(flog) << "parent process exit, mutex is released." << endl;
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
    if(hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    if(hServiceMutex) { ReleaseMutex(hServiceMutex); CloseHandle(hServiceMutex); }
    if(flog.is_open()) flog.close();
    return 0;
}
