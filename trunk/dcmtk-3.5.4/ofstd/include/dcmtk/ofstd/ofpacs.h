OFCondition MkdirRecursive(OFString& subdirectoryPath);
int GenerateLogPath(char *buf, size_t bufLen, const char *appName, const char pathSeparator);
BOOL DeleteEmptyFile(const char *filePath);