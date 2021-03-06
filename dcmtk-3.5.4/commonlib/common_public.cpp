#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/timeb.h>
#include <share.h>
#include <io.h>
#include <fcntl.h>
#include <direct.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <Aclapi.h>
#include <iostream>
#include <sstream>

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

static int signalInterruptFlag = 0;

#ifdef COMMONLIB_EXPORTS
#define SignalInterruptHandler SignalInterruptHandler_dll
COMMONLIB_API void SignalInterruptHandler_dll(int signal)
#else
#define SignalInterruptHandler SignalInterruptHandler_internal
void SignalInterruptHandler_internal(int signal)
#endif
{
  signalInterruptFlag = signal;
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API void Capture_Ctrl_C_dll()
#else
void Capture_Ctrl_C_internal()
#endif
{
  signalInterruptFlag = 0;
  signal(SIGINT, SignalInterruptHandler);
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetSignalInterruptValue_dll()
#else
int GetSignalInterruptValue_internal()
#endif
{
  return signalInterruptFlag;
}

#ifdef COMMONLIB_EXPORTS
#define GenerateTime GenerateTime_dll
COMMONLIB_API size_t GenerateTime_dll(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#else
#define GenerateTime GenerateTime_internal
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

#define DATE_FORMAT_YEAR_TO_SECOND "%Y-%m-%d %H:%M:%S"

#ifdef COMMONLIB_EXPORTS
#define time_header_out time_header_out_dll
COMMONLIB_API std::ostream& time_header_out_dll(std::ostream &os)
#else
#define time_header_out time_header_out_internal
std::ostream& time_header_out_internal(std::ostream &os)
#endif
{
	char timeBuffer[32];
	if(GenerateTime(DATE_FORMAT_YEAR_TO_SECOND, timeBuffer, sizeof(timeBuffer), NULL)) os << timeBuffer << ' ';
	return os;
}

#define ENV_PACS_TEMP "PACS_TEMP"

#ifdef COMMONLIB_EXPORTS
extern COMMONLIB_API char COMMONLIB_PACS_TEMP[MAX_PATH];
#define GetPacsTemp GetPacsTemp_dll
COMMONLIB_API const char* GetPacsTemp_dll()
#else
static char COMMONLIB_PACS_TEMP[MAX_PATH] = "";
#define GetPacsTemp GetPacsTemp_internal
const char* GetPacsTemp_internal()
#endif
{
	size_t requiredSize = 0;
    if(strlen(COMMONLIB_PACS_TEMP) == 0)
    {
	    getenv_s( &requiredSize, NULL, 0, ENV_PACS_TEMP);
	    if(requiredSize > 0)
            getenv_s(&requiredSize, COMMONLIB_PACS_TEMP, requiredSize, ENV_PACS_TEMP);
	    else
            strcpy_s(COMMONLIB_PACS_TEMP, "C:\\usr\\local\\dicom");
    }
    return COMMONLIB_PACS_TEMP;
}

#define ENV_PACS_BASE "PACS_BASE"

#ifdef COMMONLIB_EXPORTS
extern COMMONLIB_API char COMMONLIB_PACS_BASE[MAX_PATH];
#define GetPacsBase GetPacsBase_dll
COMMONLIB_API const char* GetPacsBase_dll()
#else
static char COMMONLIB_PACS_BASE[MAX_PATH] = "";
#define GetPacsBase GetPacsBase_internal
const char* GetPacsBase_internal()
#endif
{
	size_t requiredSize = 0;
    if(strlen(COMMONLIB_PACS_BASE) == 0)
    {
	    getenv_s( &requiredSize, NULL, 0, ENV_PACS_BASE);
	    if(requiredSize > 0)
            getenv_s(&requiredSize, COMMONLIB_PACS_BASE, requiredSize, ENV_PACS_BASE);
	    else
            strcpy_s(COMMONLIB_PACS_BASE, "C:\\usr\\local\\dicom");
    }
    return COMMONLIB_PACS_BASE;
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int ChangeToBasePacsSub_dll(const char *pBase, char* const pPacsBase, size_t buff_size)
#else
int ChangeToBasePacsSub_internal(const char *pBase, char* const pPacsBase, size_t buff_size)
#endif
{
    const char pacs[] = "\\pacs";
    char *path = NULL;
    if(pPacsBase)
        path = pPacsBase;
    else
    {
        path = new char[MAX_PATH];
        buff_size = MAX_PATH;
    }
    size_t base_len = strlen(pBase);
    if(base_len + sizeof(pacs) > buff_size)
    {
        if(pPacsBase == NULL) delete[] path;
        return ENOBUFS;
    }
    strncpy_s(path, buff_size, pBase, base_len);
    strncpy_s(path + base_len, buff_size - base_len, pacs, _TRUNCATE);
    errno_t en = _chdir(path);
    if(pPacsBase == NULL) delete[] path;
    return en;
}

#ifdef COMMONLIB_EXPORTS
#define displayErrorToCerr displayErrorToCerr_dll
COMMONLIB_API DWORD displayErrorToCerr_dll(const TCHAR *lpszFunction, DWORD dw, std::ostream *perrstrm)
#else
#define displayErrorToCerr displayErrorToCerr_internal
DWORD displayErrorToCerr_internal(const TCHAR *lpszFunction, DWORD dw, std::ostream *perrstrm)
#endif
{
	TCHAR *lpMsgBuf;
	//TCHAR *lpDisplayBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
	// Display the error message
	//lpDisplayBuf = (TCHAR *)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	//StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR), TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf); 
    time_header_out(perrstrm ? *perrstrm : std::cerr) << lpszFunction << " failed with error " << dw << ": " << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	//LocalFree(lpDisplayBuf);
    return dw;
}

#ifdef COMMONLIB_EXPORTS
#define DisplayErrorToFileHandle DisplayErrorToFileHandle_dll
COMMONLIB_API DWORD DisplayErrorToFileHandle_dll(TCHAR *lpszFunction, DWORD dw, HANDLE fh)
#else
#define DisplayErrorToFileHandle DisplayErrorToFileHandle_internal
DWORD DisplayErrorToFileHandle_internal(TCHAR *lpszFunction, DWORD dw, HANDLE fh)
#endif
{
	TCHAR *lpMsgBuf;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );

    std::ostringstream msgbuf;
    time_header_out(msgbuf) << lpszFunction << " failed with error " << dw << ": " << lpMsgBuf << std::endl;
    std::string msg(msgbuf.str());
    DWORD written = 0;
    WriteFile(fh, msg.c_str(), msg.length(), &written, NULL);
	LocalFree(lpMsgBuf);
    return dw;
}

FILE *create_transaction_append_file(const char *fn)
{
    FILE *fp = NULL;
    if(fn == NULL) return NULL;
    HANDLE herr = CreateFile(fn,  FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(herr == INVALID_HANDLE_VALUE)
    {
        char msg[MAX_PATH];
        sprintf_s(msg, "CreateFile(%s)", fn);
        displayErrorToCerr(msg, GetLastError(), NULL);
    }
    else
    {
        int fd = _open_osfhandle((intptr_t)herr, _O_APPEND | _O_TEXT);
        if(fd != -1)
            fp = _fdopen(fd, "a");
        else
        {
            char msg[MAX_PATH];
            sprintf_s(msg, "_open_osfhandle(fd of %s) failed", fn);
            perror(msg);
        }
    }
    return fp;
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

#ifdef COMMONLIB_EXPORTS
#define create_shared_memory_mapping create_shared_memory_mapping_dll
COMMONLIB_API void* create_shared_memory_mapping_dll(const char *mapping_name, size_t mapping_size, HANDLE *phmap, HANDLE *phfile, std::ostream *plog)
#else
#define create_shared_memory_mapping create_shared_memory_mapping_internal
void* create_shared_memory_mapping_internal(const char *mapping_name, size_t mapping_size, HANDLE *phmap, HANDLE *phfile, std::ostream *plog)
#endif
{
    if(phmap && *phmap) return NULL;
    BOOL ownerPrivilege = SetPrivilege(SE_CREATE_GLOBAL_NAME, TRUE);
    std::string seq_mapping_name_g("Global\\"), seq_mapping_name_l("Local\\");
    seq_mapping_name_g.append(mapping_name);
    seq_mapping_name_l.append(mapping_name);

    HANDLE h_file = INVALID_HANDLE_VALUE;
    //try to open global mapping
    HANDLE h_map = OpenFileMapping(FILE_MAP_WRITE, FALSE, seq_mapping_name_g.c_str());
    DWORD dw = GetLastError();
    if(h_map == NULL && !ownerPrivilege)
    {   // global mapping does not exist, try to open local mapping
        SetLastError(0);
        dw = 0;
        h_map = OpenFileMapping(FILE_MAP_WRITE, FALSE, seq_mapping_name_l.c_str());
        dw = GetLastError();
    }
    if(h_map) goto create_hmap_OK;

    size_t baselen = 0;
    char sequence_path[MAX_PATH] = "";
    DWORD sessionId = 0;
    std::string &mappingName = seq_mapping_name_g;
    if(sequence_path[0] == '\0')
    {
        const char *basedir = GetPacsBase();
        strcpy_s(sequence_path, basedir);
        baselen = strlen(sequence_path);
        if(ownerPrivilege)
        {
            sprintf_s(sequence_path + baselen, sizeof(sequence_path) - baselen, "\\temp\\%s-000.dat", mapping_name);
        }
        else
        {
            sessionId = WTSGetActiveConsoleSessionId();
            sprintf_s(sequence_path + baselen, sizeof(sequence_path) - baselen, "\\temp\\%s-%03d.dat", mapping_name, sessionId);
            mappingName = seq_mapping_name_l;
        }
    }
    
    if(phfile && *phfile) h_file = *phfile;
    if(h_file == INVALID_HANDLE_VALUE)
    {
        h_file = CreateFile(sequence_path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        dw = GetLastError();
        if(h_file == INVALID_HANDLE_VALUE)
        {
            char prefix[MAX_PATH];
            sprintf_s(prefix, "CreateFile(%s)", sequence_path);
            displayErrorToCerr(prefix, dw, plog);
        }
    }
    h_map = CreateFileMapping(h_file, NULL, PAGE_READWRITE | SEC_COMMIT, 0, mapping_size, mappingName.c_str());
    dw = GetLastError();
    if(h_map == NULL && dw != ERROR_ALREADY_EXISTS)
    {
        char msg[MAX_PATH];
        sprintf_s(msg, "create mapping %s", mappingName.c_str());
        displayErrorToCerr(msg, dw, plog);
    }
    if(h_map && ownerPrivilege)
    {
        PACL pDacl=NULL;
        PACL pNewDacl=NULL;
        PSECURITY_DESCRIPTOR pSD=NULL;
        EXPLICIT_ACCESS ea;
        dw = GetSecurityInfo(h_map, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, &pDacl, NULL, &pSD);
        ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
        ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfInheritance= NO_INHERITANCE;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
        ea.Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea.Trustee.ptstrName = "Authenticated Users";
        if(pDacl)
        {
            dw = SetEntriesInAcl(1,&ea,pDacl,&pNewDacl);
            if(dw == ERROR_SUCCESS)
                dw = SetSecurityInfo(h_map, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, NULL, NULL, pNewDacl, NULL);
        }
        if(pSD) LocalFree(pSD);
        if(pNewDacl) LocalFree(pNewDacl);
        if(dw != ERROR_SUCCESS) displayErrorToCerr("SetSecurityInfo()", dw, plog);
    }
create_hmap_OK:
    void *mem_mapped = NULL;
    if(h_map)
    {
        // MAKE_QWORD(offset_high, offset_low) must be times of sysinfo.dwAllocationGranularity
        mem_mapped = MapViewOfFile(h_map, FILE_MAP_WRITE, 0, 0, mapping_size);
        dw = GetLastError();
        if(mem_mapped == NULL)
        {
            displayErrorToCerr("MapViewOfFile()", dw, plog);
            CloseHandle(h_map);
            h_map = NULL;
        }
    }
    else displayErrorToCerr("OpenFileMapping() or CreateFileMapping()", dw, plog);

    if(phmap) *phmap = h_map;
    if(phfile) *phfile = h_file;
    return mem_mapped;
}

#ifdef COMMONLIB_EXPORTS
#define close_shared_mapping close_shared_mapping_dll
COMMONLIB_API void close_shared_mapping_dll(void *shared_mem_ptr, HANDLE h_map, HANDLE h_file)
#else
#define close_shared_mapping close_shared_mapping_internal
void close_shared_mapping_internal(void *shared_mem_ptr, HANDLE h_map, HANDLE h_file)
#endif
{
    if(shared_mem_ptr) UnmapViewOfFile(shared_mem_ptr);
    if(h_map && h_map != INVALID_HANDLE_VALUE) CloseHandle(h_map);
    if(h_file && h_file != INVALID_HANDLE_VALUE) CloseHandle(h_file);
}

typedef struct
{
    struct
    {
        struct _timeb tb;
        __time64_t diff;
    } time_part;
    int checksum;
    int get_checksum()
    {
        BYTE *vb = reinterpret_cast<BYTE*>(&this->time_part);
        int sum = 0;
        for(int i = 0; i < sizeof(time_part); ++i) sum += vb[i];
        return sum;
    }
    bool verify()
    {
        return (get_checksum() == this->checksum);
    }
} MapHistory;

#define SEQ_MUTEX_NAME "Global\\DCM_GetNextUniqueNo"
#define SEQ_MAPPING_NAME "DCM_MappingUniqueNo"
static HANDLE mutex_seq = NULL, hfile = INVALID_HANDLE_VALUE, hmap = NULL;
static MapHistory *pMapHistory = NULL;
static SYSTEM_INFO sysinfo = {0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0};

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size)
#else
int GetNextUniqueNo_internal(const char *prefix, char *pbuf, const size_t buf_size)
#endif
{
    DWORD sessionId = 0;
    const char *mappingName = SEQ_MAPPING_NAME;
    char sequence_path[MAX_PATH] = "";
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
            displayErrorToCerr("OpenMutex()", dw, NULL);
    }

    if(mutex_seq && WAIT_FAILED != WaitForSingleObject(mutex_seq, INFINITE))
    {
        owner_mutex = true;
        if(hmap == NULL)
        {
            pMapHistory = reinterpret_cast<MapHistory*>(create_shared_memory_mapping(SEQ_MAPPING_NAME, sizeof(MapHistory), &hmap, &hfile, NULL));
        }
    }
    if(pMapHistory == NULL)
    {
        fprintf(stderr, "can't asso share memory to %s, heap memory instead.\n", sequence_path);
        pMapHistory = new MapHistory;
        memset(pMapHistory, 0, sizeof(MapHistory));
    }

    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < pMapHistory->time_part.tb.time || (storeTimeThis.time == pMapHistory->time_part.tb.time && storeTimeThis.millitm <= pMapHistory->time_part.tb.millitm))
    {
        if(pMapHistory->time_part.tb.millitm == 999)
        {
            ++pMapHistory->time_part.tb.time;
            pMapHistory->time_part.tb.millitm = 0;
        }
        else
            ++pMapHistory->time_part.tb.millitm;

        pMapHistory->time_part.diff = (pMapHistory->time_part.tb.time - storeTimeThis.time) * 1000 + pMapHistory->time_part.tb.millitm - storeTimeThis.millitm;
        storeTimeThis = pMapHistory->time_part.tb;
    }
    else
    {
        pMapHistory->time_part.tb = storeTimeThis;
        pMapHistory->time_part.diff = 0;
    }
    pMapHistory->checksum = pMapHistory->get_checksum();
    if(owner_mutex) ReleaseMutex(mutex_seq);
    
    struct tm localtime;
    localtime_s(&localtime, &storeTimeThis.time);
    int buf_pos = sprintf_s(pbuf, buf_size, "%s", prefix);
    buf_pos += strftime(pbuf + buf_pos, buf_size - buf_pos, "%Y%m%d%H%M%S", &localtime);
    buf_pos += sprintf_s(pbuf + buf_pos, buf_size - buf_pos, ".%03hdT%+04d-%lld_%03d", storeTimeThis.millitm, -storeTimeThis.timezone, pMapHistory->time_part.diff, sessionId);
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
