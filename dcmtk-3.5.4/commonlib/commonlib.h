#pragma once

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
#define SET_LOCAL locale::global(locale(CHINESE_LOCAL))
#define SVC_ERROR ((DWORD)0xC0020001L)
#define SVC_INFO  ((DWORD)0x60020002L)
#define QUEUE_NAME ".\\private$\\archive"
#define REPLACE_PLACE_HOLDER "%replace%"
#define MOVE_PLACE_HOLDER "%move%"
#define ARCHIVE_STUDY "Archive Study"
#define ARCHIVE_INSTANCE "Archive Instance"
#ifdef _WIN32
#define DATE_FORMAT_YEAR_TO_SECOND "%Y-%m-%d %H:%M:%S"
#define DATE_FORMAT_COMPACT "%Y%m%d%H%M%S"
#else
#define DATE_FORMAT_YEAR_TO_SECOND "%F %T"
#endif

const char CHARSET_ISO_IR_100[] = "ISO_IR 100", CHARSET_GB18030[] = "GB18030",
  ADD_DEFAULT_CHARSET[] = "Add default character set ", UNKNOWN_CHARSET[] = "Unknown character set ", OVERRIDE_BY[] = " is override by ";

void displayErrorToCerr(TCHAR *lpszFunction);
int GetSignalInterruptValue();
void SignalInterruptHandler(int signal);
void Capture_Ctrl_C();
bool IsASCII(const char *str);
char *rtrim(char *s, int maxLen = -1);
LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime);
bool MkdirRecursive(const char*);
bool prepareFileDir(const char *path);
int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
BOOL DeleteEmptyFile(const char *filePath);
long generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath, bool deleteSourceCSV = false);
time_t dcmdate2tm(int dcmdate);
bool generateStudyXML(const char *line, std::ostream &xmlStream, bool isEncapsulated = false);
bool RedirectMessageLabelEqualWith(char *label, char *body, const int bodyLength, const char *equalWith, const char *queueName);
bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName);
bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd);
errno_t setEnvParentPID();
int generateTime(const char *format, char *timeBuffer, size_t bufferSize);
void changeWorkingDirectory(int argc, char **argv);
int hashCode(const char *);
int hashCodeW(const wchar_t *);