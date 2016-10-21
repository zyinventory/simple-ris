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

#define STRING_TRIM(str) { _locale_t loc = _create_locale(LC_ALL, "");\
    str.erase(find_if(str.rbegin(), str.rend(), std::not1(std::bind2nd(std::ptr_fun<int, _locale_t, int>(::_isspace_l), loc))).base(), str.end());\
    str.erase(str.begin(), find_if(str.begin(), str.end(), std::not1(std::bind2nd(std::ptr_fun<int, _locale_t, int>(::_isspace_l), loc))));\
    _free_locale(loc); }

#define THROW_HR_TO_COM_ERROR(hr, polemsg) {\
    CComQIPtr<ICreateErrorInfo> pce;\
    CComQIPtr<IErrorInfo> pei;\
    CreateErrorInfo(&pce);\
    pce->SetDescription(polemsg);\
    pce->QueryInterface(IID_IErrorInfo, (LPVOID*)&pei);\
    throw _com_error(hr, pei, true);\
}\

#ifdef RETURN_HRESULT

#define CATCH_COM_ERROR(func_name) \
catch(const _com_error &ex) \
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << ex.ErrorMessage();\
    _bstr_t desc = ex.Description();\
    if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    errstrm << std::endl;\
    hr = ex.Error();\
}\
catch(const runtime_error &re)\
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << re.what() << std::endl;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        errstrm << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        errstrm << std::endl;\
        hr = hrdw;\
    }\
    else hr = E_FAIL;\
}\
catch(const exception &e)\
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << e.what() << std::endl;\
    hr = E_FAIL;\
}\
catch(...)\
{\
    time_header_out(errstrm) << func_name << " encouter unknown error at " << __FILE__ << " " << __LINE__;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        errstrm << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        hr = hrdw;\
    }\
    else hr = E_FAIL;\
    errstrm << std::endl;\
}\

#else

#define CATCH_COM_ERROR(func_name, errstrm) \
catch(const _com_error &ex) \
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << ex.ErrorMessage();\
    _bstr_t desc = ex.Description();\
    if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    errstrm << std::endl;\
}\
catch(const runtime_error &re)\
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << re.what() << std::endl;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        errstrm << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
        errstrm << std::endl;\
    }\
}\
catch(const exception &e)\
{\
    time_header_out(errstrm) << func_name << " failed at " << __FILE__ << " " << __LINE__ << ": " << e.what() << std::endl;\
}\
catch(...)\
{\
    time_header_out(errstrm) << func_name << " encouter unknown error at " << __FILE__ << " " << __LINE__;\
    HRESULT hrdw = AtlHresultFromLastError();\
    if(hrdw != S_OK)\
    {\
        _com_error ce(hrdw);\
        errstrm << ": " << ce.ErrorMessage();\
        _bstr_t desc = ce.Description();\
        if(desc.length()) cerr << ", " << (LPCSTR)desc;\
    }\
    errstrm << std::endl;\
}\

#endif

const char CHARSET_ISO_IR_100[] = "ISO_IR 100", CHARSET_ISO_IR_100_ALIAS[] = "ISO 2022 IR 100",
    CHARSET_ISO_IR_87[]  = "ISO_IR 87",  CHARSET_ISO_IR_87_ALIAS[]  = "ISO 2022 IR 87",
    CHARSET_ISO_IR_159[] = "ISO_IR 159", CHARSET_ISO_IR_159_ALIAS[] = "ISO 2022 IR 159",
    CHARSET_ISO_IR_149[] = "ISO_IR 149", CHARSET_ISO_IR_149_ALIAS[] = "ISO 2022 IR 149",
    CHARSET_ISO_IR_165[] = "ISO_IR 165", CHARSET_ISO_IR_165_ALIAS[] = "ISO 2022 IR 165",
    CHARSET_UTF8[]       = "ISO_IR 192", CHARSET_UTF8_ALIAS[]       = "ISO 2022 IR 192", CHARSET_GB18030[] = "GB18030";

extern COMMONLIB_API char COMMONLIB_PACS_BASE[MAX_PATH];
extern COMMONLIB_API bool CommonlibBurnOnce, CommonlibInstanceUniquePath;

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
COMMONLIB_API int generateStudyTxt(const char *study_uid, std::ostream &errstrm);
COMMONLIB_API int generateStudyJDF(const char *tag, const char *tagValue, std::ostream &errstrm, const char *media = MEDIA_AUTO);
COMMONLIB_API long generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath, bool deleteSourceCSV = false);
COMMONLIB_API time_t dcmdate2tm(int dcmdate);
COMMONLIB_API bool generateStudyXML(const char *line, std::ostream &xmlStream, bool isEncapsulated = false);
COMMONLIB_API bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd);
COMMONLIB_API bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName);
COMMONLIB_API bool EnsureQueueExist(const char *queuePath);
COMMONLIB_API bool DeleteQueue(const char *queueName);
COMMONLIB_API errno_t setEnvParentPID();
COMMONLIB_API __int64 HashStrW(const wchar_t *s, char *buffer = NULL, size_t buffer_size = 0);
COMMONLIB_API __int64 HashStr(const char *s, char *buffer = NULL, size_t buffer_size = 0);
COMMONLIB_API long long diskUsage(const char *pacsBase, const char *studyUID);
COMMONLIB_API bool DeleteSubTree(const char *dirpath, std::ostream *ostrm = NULL);
COMMONLIB_API bool DeleteTree(const char *dirpath, std::ostream *ostrm = NULL);
COMMONLIB_API bool deleteStudyFromIndex(const char *mode, const char *modeValue, const char *studyUid);
COMMONLIB_API size_t extractStudyUid(char *buffer, const size_t bufferSize, const wchar_t *body);
COMMONLIB_API errno_t SeriesInstancePath(const char *series, const std::string &instance, char *outputBuffer, size_t bufLen, char pathSeparator = '\\');
COMMONLIB_API int StatusXml(const char *statusFlag, const char *ini_path, int licenseCnt, std::ostream &outputbuf);
COMMONLIB_API bool EncodeBase32(const char *src, char *enc, size_t enc_buf_size);
COMMONLIB_API bool DecodeBase32(const char *src, char *dec, size_t dec_buf_size);
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size);
COMMONLIB_API void ReleaseUniqueNoResource();

COMMONLIB_API size_t in_process_sequence_dll(char *buff, size_t buff_size, const char *prefix);
COMMONLIB_API int LoadSettings(const char *iniPath, std::ostream &oslog, bool opt_verbose);
COMMONLIB_API size_t GetSetting(const std::string &key, char *buff, size_t buff_size);
COMMONLIB_API const char *GetGenerateIndexLog();
COMMONLIB_API void ClearGenerateIndexLog();
COMMONLIB_API bool SelectValidPublisher(const char *ini_path, char *valid_publisher, size_t buff_size, bool opt_verbose);
#define TryPublishJDF_NoValidPrinter    2
#define TryPublishJDF_PublishOK     1
#define TryPublishJDF_PublisherBusy 0
#define TryPublishJDF_SrcOpenError  -1
#define TryPublishJDF_SrcMarkError  -2
COMMONLIB_API int TryPublishJDF(bool opt_verbose = false, const char *filename = NULL);
COMMONLIB_API int UTF8ToGBK(const char *lpUTF8Str, char *lpGBKStr, int nGBKStrLen);
COMMONLIB_API int GBKToUTF8(const char *lpGBKStr, char *lpUTF8Str, int nUTF8StrLen);
COMMONLIB_API int AutoCharToGBK(char *buff, int nGBKStrLen, const char *instr);
COMMONLIB_API int ValidateGBK(unsigned char *buff, int max_len);
COMMONLIB_API size_t normalize_dicom_date(size_t buff_len, char *buff, const char *studyDate);

// common_public.cpp
#ifndef GetSignalInterruptValue
#define GetSignalInterruptValue GetSignalInterruptValue_dll
#endif
COMMONLIB_API int GetSignalInterruptValue_dll();

#ifndef SignalInterruptHandler
#define SignalInterruptHandler SignalInterruptHandler_dll
#endif
COMMONLIB_API void SignalInterruptHandler_dll(int signal);

#ifndef Capture_Ctrl_C
#define Capture_Ctrl_C Capture_Ctrl_C_dll
#endif
COMMONLIB_API void Capture_Ctrl_C_dll();

#ifndef GetPacsBase
#define GetPacsBase GetPacsBase_dll
#endif
COMMONLIB_API const char* GetPacsBase_dll();

#ifndef ChangeToPacsWebSub
#define ChangeToPacsWebSub ChangeToPacsWebSub_dll
#endif
COMMONLIB_API int ChangeToPacsWebSub_dll(char *pPacsBase, size_t buff_size);

#ifndef time_header_out
#define time_header_out time_header_out_dll
#endif
COMMONLIB_API std::ostream& time_header_out_dll(std::ostream &os);

#ifndef GenerateTime
#define GenerateTime GenerateTime_dll
#endif
COMMONLIB_API size_t GenerateTime_dll(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now = NULL);

#ifndef displayErrorToCerr
#define displayErrorToCerr displayErrorToCerr_dll
#endif
COMMONLIB_API DWORD displayErrorToCerr_dll(const TCHAR *lpszFunction, DWORD dw, std::ostream *perrstrm = NULL);

#ifndef DisplayErrorToFileHandle
#define DisplayErrorToFileHandle DisplayErrorToFileHandle_dll
#endif
COMMONLIB_API DWORD DisplayErrorToFileHandle_dll(TCHAR *lpszFunction, DWORD dw, HANDLE fh);

#ifndef create_shared_memory_mapping
#define create_shared_memory_mapping create_shared_memory_mapping_dll
#endif
COMMONLIB_API void* create_shared_memory_mapping_dll(const char *mapping_name, size_t mapping_size, HANDLE *phmap, HANDLE *phfile, std::ostream *plog);

#ifndef close_shared_mapping
#define close_shared_mapping close_shared_mapping_dll
#endif
COMMONLIB_API void close_shared_mapping_dll(void *shared_mem_ptr, HANDLE h_map, HANDLE h_file);
// common_public.cpp

#ifdef _DEBUG

COMMONLIB_API int test_for_make_index(bool verbose);

#endif //_DEBUG
