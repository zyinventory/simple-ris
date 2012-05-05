#pragma once

int GetSignalInterruptValue();
void SignalInterruptHandler(int signal);
void Capture_Ctrl_C();
bool IsASCII(const char *str);
char *rtrim(char *s, int maxLen = -1);
errno_t GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
LONGLONG GetFileInfo(const char *filePath, PSYSTEMTIME localTime);
void DeleteEmptyFile(const char *filePath);
