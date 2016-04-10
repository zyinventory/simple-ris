#pragma once

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
#define SET_LOCAL locale::global(locale(CHINESE_LOCAL))
#define QUEUE_NAME ".\\private$\\archive"
#define REPLACE_PLACE_HOLDER "%replace%"
#define MOVE_PLACE_HOLDER "%move%"
#define ARCHIVE_STUDY "Archive Study"
#define ARCHIVE_STUDY_NOT_INTEGRITY "Archive Study Not Integrity"
#define ARCHIVE_INSTANCE "Archive Instance"
#define NOTIFY_COMPRESSED "compressed "
#define NOTIFY_END_OF_STUDY "dcmmkdir"
#define NOTIFY_STUDY_NOT_INTEGRITY "dcmmkdir not integrity"
#define MEDIA_AUTO "AUTO"
#define MEDIA_CD "CD"
#define MEDIA_DVD "DVD"
#define MEDIA_DVD_DL "DVD-DL"
#define MEDIA_BD "BD"
#define MEDIA_BD_DL "BD-DL"
//#define MQ_PRIORITY_PROTOCOL	7
//#define MQ_PRIORITY_ARCHIVING	6
#define MQ_PRIORITY_RECEIVED	3
#define MQ_PRIORITY_DCMMKDIR	0
#ifdef _WIN32
#define DATE_FORMAT_YEAR_TO_SECOND "%Y-%m-%d %H:%M:%S"
#define DATE_FORMAT_COMPACT "%Y%m%d%H%M%S"
#else
#define DATE_FORMAT_YEAR_TO_SECOND "%F %T"
#endif
#define SERIES_INSTANCE_PATH_MAX 54  // series_hash(8) + instance_group(9) * 5 + \0(1)
#define TDB_STATUS "..\\orders\\TDBStatus.txt"
#define PIPE_READY_MESSAGE "PIPE_READY"

#ifdef RETURN_HRESULT

#define CATCH_COM_ERROR(func_name) \
catch(const _com_error &ex) \
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << ex.ErrorMessage();\
    _bstr_t desc = ex.Description();\
    if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    std::cerr << std::endl;\
    hr = ex.Error();\
}\
catch(const runtime_error &re)\
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << re.what() << std::endl;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        std::cerr << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        std::cerr << std::endl;\
        hr = hrdw;\
    }\
    else hr = E_FAIL;\
}\
catch(const exception &e)\
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << e.what() << std::endl;\
    hr = E_FAIL;\
}\
catch(...)\
{\
    std::cerr << func_name << " encouter unknown error at " << __FILE__ << " " << __LINE__;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        std::cerr << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        hr = hrdw;\
    }\
    else hr = E_FAIL;\
    std::cerr << std::endl;\
}\

#else

#define CATCH_COM_ERROR(func_name) \
catch(const _com_error &ex) \
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << ex.ErrorMessage();\
    _bstr_t desc = ex.Description();\
    if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    std::cerr << std::endl;\
}\
catch(const runtime_error &re)\
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << re.what() << std::endl;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        std::cerr << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        std::cerr << std::endl;\
    }\
}\
catch(const exception &e)\
{\
    std::cerr << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << e.what() << std::endl;\
}\
catch(...)\
{\
    std::cerr << func_name << " encouter unknown error at " << __FILE__ << " " << __LINE__;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        std::cerr << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    }\
    std::cerr << std::endl;\
}\

#endif

const char CHARSET_ISO_IR_100[] = "ISO_IR 100", CHARSET_GB18030[] = "GB18030",
  ADD_DEFAULT_CHARSET[] = "Add default character set ", UNKNOWN_CHARSET[] = "Unknown character set ", OVERRIDE_BY[] = " is override by ";

extern COMMONLIB_API bool CommonlibBurnOnce, CommonlibInstanceUniquePath;

COMMONLIB_API DWORD displayErrorToCerr(const TCHAR *lpszFunction, DWORD gle, std::ostream *perrstrm = NULL);
COMMONLIB_API DWORD DisplayErrorToFileHandle(TCHAR *lpszFunction, DWORD dw, HANDLE fh);
COMMONLIB_API int GetSignalInterruptValue();
COMMONLIB_API void SignalInterruptHandler(int signal);
COMMONLIB_API void Capture_Ctrl_C();
COMMONLIB_API bool IsASCII(const char *str);
COMMONLIB_API const char *trim_const(const char *s, int maxStrLen, const char **ptail);
COMMONLIB_API char *rtrim(char *s, int maxLen = INT_MAX - 1);
COMMONLIB_API char *trim(char *s, int maxStrLen = INT_MAX - 1);
COMMONLIB_API LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime = NULL);
COMMONLIB_API bool MkdirRecursive(const char*);
COMMONLIB_API bool PrepareFileDir(const char *path);
COMMONLIB_API int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
COMMONLIB_API BOOL DeleteEmptyFile(const char *filePath);
COMMONLIB_API const char* detectMediaType(size_t *pSize);
COMMONLIB_API int generateStudyJDF(const char *tag, const char *tagValue, std::ostream &errstrm, const char *media = MEDIA_AUTO);
COMMONLIB_API long generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath, bool deleteSourceCSV = false);
COMMONLIB_API time_t dcmdate2tm(int dcmdate);
COMMONLIB_API bool generateStudyXML(const char *line, std::ostream &xmlStream, bool isEncapsulated = false);
COMMONLIB_API bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd);
COMMONLIB_API bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName);
COMMONLIB_API bool EnsureQueueExist(const char *queuePath);
COMMONLIB_API bool DeleteQueue(const char *queueName);
COMMONLIB_API errno_t setEnvParentPID();
COMMONLIB_API size_t GenerateTime(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now = NULL);
COMMONLIB_API char* GetPacsBase();
COMMONLIB_API int ChangeToPacsWebSub(char *pPacsBase, size_t buff_size, const char *subdir = NULL);
COMMONLIB_API int changeWorkingDirectory(int argc, char **argv, char *pPacsBase = NULL);
COMMONLIB_API __int64 HashStrW(const wchar_t *s, char *buffer = NULL, size_t buffer_size = 0);
COMMONLIB_API __int64 HashStr(const char *s, char *buffer = NULL, size_t buffer_size = 0);
COMMONLIB_API long long diskUsage(const char *pacsBase, const char *studyUID);
COMMONLIB_API bool DeleteSubTree(const char *dirpath, std::ostream *ostrm = NULL);
COMMONLIB_API bool DeleteTree(const char *dirpath, std::ostream *ostrm = NULL);
COMMONLIB_API bool deleteStudyFromIndex(const char *mode, const char *modeValue, const char *studyUid);
COMMONLIB_API size_t extractStudyUid(char *buffer, const size_t bufferSize, const wchar_t *body);
COMMONLIB_API errno_t SeriesInstancePath(const char *series, const std::string &instance, char *outputBuffer, size_t bufLen, char pathSeparator = '\\');
COMMONLIB_API bool SelectValidPublisher(const char *ini_path, std::string &valid_publisher);
COMMONLIB_API int StatusXml(const char *statusFlag, const char *ini_path, int licenseCnt, std::ostream &outputbuf);
COMMONLIB_API bool EncodeBase32(const char *src, char *enc, size_t enc_buf_size);
COMMONLIB_API bool DecodeBase32(const char *src, char *dec, size_t dec_buf_size);
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size);
COMMONLIB_API void ReleaseUniqueNoResource();

COMMONLIB_API int scp_store_main_loop(const char *sessionId, bool verbose);
COMMONLIB_API size_t in_process_sequence_dll(char *buff, size_t buff_size, const char *prefix);

#ifdef _DEBUG

COMMONLIB_API int test_for_make_index(const char *pacs_base, bool verbose);

#endif //_DEBUG
