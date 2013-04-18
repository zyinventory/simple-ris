#pragma once

#define CHINESE_LOCAL "chinese"  // full name: Chinese_People's Republic of China.936, posix: zh_CN.GBK
#define SET_LOCAL locale::global(locale(locale(CHINESE_LOCAL)))

const char CHARSET_ISO_IR_100[] = "ISO_IR 100", CHARSET_GB18030[] = "GB18030",
  ADD_DEFAULT_CHARSET[] = "Add default character set ", UNKNOWN_CHARSET[] = "Unknown character set ", OVERRIDE_BY[] = " is override by ";

int GetSignalInterruptValue();
void SignalInterruptHandler(int signal);
void Capture_Ctrl_C();
bool IsASCII(const char *str);
char *rtrim(char *s, int maxLen = -1);
LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime);
bool MkdirRecursive(const char*);
int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
BOOL DeleteEmptyFile(const char *filePath);
long generateIndex(char *inputFile, const char *paramBaseUrl, const char *archPath, const char *indPath);
time_t dcmdate2tm(int dcmdate);
bool generateStudyXML(const char *line, std::ostream &xmlStream);
bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd);
