#define SVC_ERROR ((DWORD)0xC0020001L)
#define SVC_INFO  ((DWORD)0x60020002L)
#define SERVICE_NAME "SmartPACSStore"

void changeWorkingDirectory(int argc, char **argv);
void ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void SvcReportEvent(LPCTSTR szFunction, WORD eventType = EVENTLOG_ERROR_TYPE, DWORD eventId = SVC_ERROR);
bool WINAPI SvcInit(DWORD dwWaitHint);
