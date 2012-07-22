#pragma once
const char CHARSET_ISO_IR_100[] = "ISO_IR 100", CHARSET_GB18030[] = "GB18030",
  ADD_DEFAULT_CHARSET[] = "Add default character set ", UNKNOWN_CHARSET[] = "Unknown character set ", OVERRIDE_BY[] = " is override by ";
const char *GetErrorMessage();
const char *GetErrorModuleName();
void logError(std::ostream& outputStream);
int GetSignalInterruptValue();
void SignalInterruptHandler(int signal);
void Capture_Ctrl_C();
bool IsASCII(const char *str);
char *rtrim(char *s, int maxLen = -1);
errno_t GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime);
BOOL DeleteEmptyFile(const char *filePath);
