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
#include <Aclapi.h>

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
COMMONLIB_API DWORD displayErrorToCerr(const TCHAR *lpszFunction, DWORD dw)
#else
#define displayErrorToCerr_public displayErrorToCerr_internal
static DWORD displayErrorToCerr_internal(const TCHAR *lpszFunction, DWORD dw)
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
    return dw;
}

#ifdef COMMONLIB_EXPORTS
#define DisplayErrorToFileHandle_public DisplayErrorToFileHandle
COMMONLIB_API DWORD DisplayErrorToFileHandle(TCHAR *lpszFunction, DWORD dw, HANDLE fh)
#else
#define DisplayErrorToFileHandle_public DisplayErrorToFileHandle_internal
static DWORD DisplayErrorToFileHandle_internal(TCHAR *lpszFunction, DWORD dw, HANDLE fh)
#endif
{
	TCHAR *lpMsgBuf;
	TCHAR *lpDisplayBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	// Display the error message
	lpDisplayBuf = (TCHAR *)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	sprintf_s(lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s\r\n"), lpszFunction, dw, lpMsgBuf);
    DWORD written = 0;
    WriteFile(fh, lpDisplayBuf, strlen(lpDisplayBuf), &written, NULL);
	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
    return dw;
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
        //fprintf(stderr, "LookupPrivilegeValue error: %u\n", GetLastError() );
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
        //fprintf(stderr, "AdjustTokenPrivileges error: %u\n", GetLastError() );
        CloseHandle(hToken);
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        //fprintf(stderr, "The token does not have the specified privilege. \n");
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
static const char *mappingName = SEQ_MAPPING_NAME_G;
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
    if(sysinfo.dwPageSize == 0) GetSystemInfo(&sysinfo);

    bool owner_mutex = false;
    DWORD dw = 0;
    if(mutex_seq == NULL)
    {
        mutex_seq = OpenMutex(SYNCHRONIZE, FALSE, SEQ_MUTEX_NAME);
        dw = GetLastError();
        if(mutex_seq == NULL && dw == ERROR_FILE_NOT_FOUND)
        {
            mutex_seq = CreateMutex(NULL, FALSE, SEQ_MUTEX_NAME);
            dw = GetLastError();
        }
        if(mutex_seq == NULL)
            displayErrorToCerr_public("OpenMutex()", dw);
    }

    if(mutex_seq && WAIT_FAILED != WaitForSingleObject(mutex_seq, INFINITE))
    {
        owner_mutex = true;
        if(hmap == NULL)
        {
            BOOL ownerPrivilege = SetPrivilege(SE_CREATE_GLOBAL_NAME, TRUE);
            //try to open global mapping
            hmap = OpenFileMapping(FILE_MAP_WRITE, FALSE, SEQ_MAPPING_NAME_G);
            dw = GetLastError();
            if(hmap == NULL && !ownerPrivilege)
            {   // global mapping does not exist, try to open local mapping
                SetLastError(0);
                dw = 0;
                hmap = OpenFileMapping(FILE_MAP_WRITE, FALSE, SEQ_MAPPING_NAME_L);
                dw = GetLastError();
            }
            if(hmap) goto hmap_OK;

            size_t baselen = 0;
            if(sequence_path[0] == '\0')
            {
                char *basedir = GetPacsBase_public();
                strcpy_s(sequence_path, basedir);
                baselen = strlen(sequence_path);
                if(ownerPrivilege)
                {
                    strcpy_s(sequence_path + baselen, sizeof(sequence_path) - baselen, "\\temp\\sequence-000.dat");
                    mappingName =  SEQ_MAPPING_NAME_G;
                }
                else
                {
                    sessionId = WTSGetActiveConsoleSessionId();
                    sprintf_s(sequence_path + baselen, sizeof(sequence_path) - baselen, "\\temp\\sequence-%03d.dat", sessionId);
                    mappingName = SEQ_MAPPING_NAME_L;
                }
            }

            if(hfile == INVALID_HANDLE_VALUE)
            {
                hfile = CreateFile(sequence_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                dw = GetLastError();
                if(hfile == INVALID_HANDLE_VALUE)
                {
                    char prefix[MAX_PATH];
                    sprintf_s(prefix, "CreateFile(%s)", sequence_path);
                    displayErrorToCerr_public(prefix, dw);
                }
            }
            hmap = CreateFileMapping(hfile, NULL, PAGE_READWRITE | SEC_COMMIT, 0, sysinfo.dwAllocationGranularity, mappingName);
            dw = GetLastError();
            if(hmap == NULL && dw != ERROR_ALREADY_EXISTS)
            {
                char msg[MAX_PATH];
                sprintf_s(msg, "create mapping %s", mappingName);
                displayErrorToCerr_public(msg, dw);
            }
            if(hmap && ownerPrivilege)
            {
                PACL pDacl=NULL;
                PACL pNewDacl=NULL;
                PSECURITY_DESCRIPTOR pSD=NULL;
                EXPLICIT_ACCESS ea;
                dw = GetSecurityInfo(hmap,SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION,NULL,NULL,&pDacl,NULL,&pSD);
                ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
                ea.grfAccessPermissions = GENERIC_WRITE;
                ea.grfAccessMode = GRANT_ACCESS;
                ea.grfInheritance= NO_INHERITANCE;
                ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
                ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
                ea.Trustee.ptstrName = "Users";
                if(pDacl)
                {
                    dw = SetEntriesInAcl(1,&ea,pDacl,&pNewDacl);
                    if(dw == ERROR_SUCCESS)
                        dw = SetSecurityInfo(hmap, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL);
                }
                if(pSD) LocalFree(pSD);
                if(pNewDacl) LocalFree(pNewDacl);
                if(dw != ERROR_SUCCESS)
                    displayErrorToCerr_public("SetSecurityInfo()", dw);
            }
hmap_OK:
            if(hmap)
            {
                if(pMapHistory) delete pMapHistory;
                pMapHistory = static_cast<MapHistory*>(MapViewOfFile(hmap, FILE_MAP_WRITE, 0, 0, sizeof(MapHistory)));
                dw = GetLastError();
                if(pMapHistory == NULL)
                {
                    displayErrorToCerr_public("MapViewOfFile()", dw);
                    CloseHandle(hmap);
                    hmap = NULL;
                }
                else if(! pMapHistory->verify())
                {
                    fputs("pMapHistory is damaged\n", stderr);
                    memset(pMapHistory, 0, sizeof(MapHistory));
                }
            }
            else
                displayErrorToCerr_public("OpenFileMapping() or CreateFileMapping()", dw);
        }
    }
    if(pMapHistory == NULL)
    {
        fprintf(stderr, "can't asso share memory to %s, heap memory instead.\n", sequence_path);
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
    pMapHistory->checksum = pMapHistory->get_checksum();
    if(owner_mutex) ReleaseMutex(mutex_seq);
    
    struct tm localtime;
    localtime_s(&localtime, &storeTimeThis.time);
    int buf_pos = sprintf_s(pbuf, buf_size, "%s", prefix);
    buf_pos += strftime(pbuf + buf_pos, buf_size - buf_pos, "%Y%m%d%H%M%S", &localtime);
    buf_pos += sprintf_s(pbuf + buf_pos, buf_size - buf_pos, ".%03hdT%+04d-%lld_%03d", storeTimeThis.millitm, -storeTimeThis.timezone, pMapHistory->diff, sessionId);
    return buf_pos;
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
