#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <share.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <errno.h>
#include <windows.h>

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API size_t GenerateTime(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#else
size_t GenerateTime_internal(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#endif
{
	time_t t, *pt = time_now ? time_now : &t;
	*pt = time( NULL );
	struct tm tmp;
	if( localtime_s( &tmp, pt ) == 0 )
		return strftime(timeBuffer, bufferSize, format, &tmp);
	else
		return 0;
}

#define ENV_PACS_BASE "PACS_BASE"
static char PACS_BASE_CACHE[MAX_PATH] = "";

#ifdef COMMONLIB_EXPORTS
#define GetPacsBase_public GetPacsBase
COMMONLIB_API char* GetPacsBase()
#else
#define GetPacsBase_public GetPacsBase_internal
static char* GetPacsBase_internal()
#endif
{
	size_t requiredSize = strlen(PACS_BASE_CACHE) + 1;
    if(requiredSize == 1)
    {
	    getenv_s( &requiredSize, NULL, 0, ENV_PACS_BASE);
	    if(requiredSize > 0)
            getenv_s(&requiredSize, PACS_BASE_CACHE, requiredSize, ENV_PACS_BASE);
	    else
            strcpy_s(PACS_BASE_CACHE, sizeof(PACS_BASE_CACHE), "C:\\usr\\local\\dicom");
    }
    return PACS_BASE_CACHE;
}

#ifdef COMMONLIB_EXPORTS
#define displayErrorToCerr_public displayErrorToCerr
COMMONLIB_API void displayErrorToCerr(TCHAR *lpszFunction, DWORD dw)
#else
#define displayErrorToCerr_public displayErrorToCerr_internal
static void displayErrorToCerr_internal(TCHAR *lpszFunction, DWORD dw)
#endif
{
	TCHAR *lpMsgBuf;
	//TCHAR *lpDisplayBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	// Display the error message
	//lpDisplayBuf = (TCHAR *)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	//StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf); 
	fprintf(stderr, TEXT("%s failed with error %d: %s\n"), lpszFunction, dw, lpMsgBuf); 
	LocalFree(lpMsgBuf);
	//LocalFree(lpDisplayBuf);
}

static BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return FALSE;

    if ( !LookupPrivilegeValue(
        NULL,            // lookup privilege on local system
        lpszPrivilege,   // privilege to lookup
        &luid ) )        // receives LUID of privilege
    {
        fprintf(stderr, "LookupPrivilegeValue error: %u\n", GetLastError() );
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL) )
    {
        fprintf(stderr, "AdjustTokenPrivileges error: %u\n", GetLastError() );
        CloseHandle(hToken);
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        fprintf(stderr, "The token does not have the specified privilege. \n");
        CloseHandle(hToken);
        return FALSE;
    }
    CloseHandle(hToken);
    return TRUE;
}

typedef struct
{
    struct _timeb tb;
    __time64_t diff;
    int checksum;
    int get_checksum()
    {
        BYTE *vb = reinterpret_cast<BYTE*>(&this->tb);
        int sum = 0;
        for(int i = 0; i < sizeof(_timeb); ++i) sum += vb[i];
        vb = reinterpret_cast<BYTE*>(&this->diff);
        for(int i = 0; i < sizeof(__time64_t); ++i) sum += vb[i];
        return sum;
    }
    bool verify()
    {
        return (get_checksum() == this->checksum);
    }
} MapHistory;

#define SEQ_MUTEX_NAME "Global\\DCM_GetNextUniqueNo"
#define SEQ_MAPPING_NAME_G "Global\\DCM_MappingUniqueNo"
#define SEQ_MAPPING_NAME_L "Local\\DCM_MappingUniqueNo"
static const char *mappingName = SEQ_MAPPING_NAME_L;
static char sequence_path[MAX_PATH] = "";
static HANDLE mutex_seq = NULL, hfile = INVALID_HANDLE_VALUE, hmap = NULL;
static MapHistory *pMapHistory = NULL;
static SYSTEM_INFO sysinfo = {0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0};
static DWORD sessionId = 0;

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size)
#else
int GetNextUniqueNo_internal(const char *prefix, char *pbuf, const size_t buf_size)
#endif
{
    struct _timeb storeTimeThis;
    if(buf_size < 40) return -1;
    if(sequence_path[0] == '\0')
    {
        char *basedir = GetPacsBase_public();
        mappingName = SetPrivilege(SE_CREATE_GLOBAL_NAME, TRUE) ? SEQ_MAPPING_NAME_G : SEQ_MAPPING_NAME_L;
        sessionId = WTSGetActiveConsoleSessionId();
        strcpy_s(sequence_path, basedir);
        size_t baselen = strlen(sequence_path);
        sprintf_s(sequence_path + baselen, sizeof(sequence_path) - baselen, "\\temp\\sequence-%03d.dat", sessionId);
    }
    if(sysinfo.dwPageSize == 0) GetSystemInfo(&sysinfo);

    bool owner_mutex = false;
    if(mutex_seq == NULL)
    {
        mutex_seq = OpenMutex(SYNCHRONIZE, FALSE, SEQ_MUTEX_NAME);
        if(mutex_seq == NULL && GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            displayErrorToCerr_public("OpenMutex()", ERROR_FILE_NOT_FOUND);
            mutex_seq = CreateMutex(NULL, FALSE, SEQ_MUTEX_NAME);
            displayErrorToCerr_public("CreateMutex()", GetLastError());
        }
    }

    if(mutex_seq && WAIT_FAILED != WaitForSingleObject(mutex_seq, INFINITE))
    {
        owner_mutex = true;
        if(hfile == INVALID_HANDLE_VALUE)
        {
            hfile = CreateFile(sequence_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if(hfile == INVALID_HANDLE_VALUE)
                displayErrorToCerr_public("CreateFile()", GetLastError());
        }
        if(hmap == NULL)
        {
            hmap = CreateFileMapping(hfile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, sysinfo.dwAllocationGranularity, mappingName);
            displayErrorToCerr_public("child create mapping", GetLastError());
            if(hmap)
            {
                if(pMapHistory) delete pMapHistory;
                pMapHistory = static_cast<MapHistory*>(MapViewOfFile(hmap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MapHistory)));
                if(pMapHistory == NULL)
                {
                    displayErrorToCerr_public("MapViewOfFile()", GetLastError());
                    CloseHandle(hmap);
                    hmap = NULL;
                }
                else if(! pMapHistory->verify())
                    memset(pMapHistory, 0, sizeof(MapHistory));
            }
            else
                displayErrorToCerr_public("CreateFileMapping()", GetLastError());
        }
    }
    if(pMapHistory == NULL)
    {
        fprintf(stderr, "can't asso share memory to %s, new MapHistory instead.\n", sequence_path);
        pMapHistory = new MapHistory;
        memset(pMapHistory, 0, sizeof(MapHistory));
    }

    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < pMapHistory->tb.time || (storeTimeThis.time == pMapHistory->tb.time && storeTimeThis.millitm <= pMapHistory->tb.millitm))
    {
        if(pMapHistory->tb.millitm == 999)
        {
            ++pMapHistory->tb.time;
            pMapHistory->tb.millitm = 0;
        }
        else
            ++pMapHistory->tb.millitm;

        pMapHistory->diff = (pMapHistory->tb.time - storeTimeThis.time) * 1000 + pMapHistory->tb.millitm - storeTimeThis.millitm;
        storeTimeThis = pMapHistory->tb;
    }
    else
    {
        pMapHistory->tb = storeTimeThis;
        pMapHistory->diff = 0;
    }

    if(owner_mutex) ReleaseMutex(mutex_seq);
    return sprintf_s(pbuf, buf_size, "%s%lld%03hd-%lld_%03d", prefix, storeTimeThis.time, storeTimeThis.millitm, pMapHistory->diff, sessionId);
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API void ReleaseUniqueNoResource()
#else
void ReleaseUniqueNoResource_internal()
#endif
{
    if(pMapHistory)
    {
        if(hmap)
        {
            pMapHistory->checksum = pMapHistory->get_checksum();
            UnmapViewOfFile(pMapHistory);
            CloseHandle(hmap);
            hmap = NULL;
        }
        else
            delete pMapHistory;
        pMapHistory = NULL;
    }
    if(hfile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfile);
        hfile = INVALID_HANDLE_VALUE;
    }
    if(mutex_seq)
    {
        ReleaseMutex(mutex_seq);
        CloseHandle(mutex_seq);
        mutex_seq = NULL;
    }
}
