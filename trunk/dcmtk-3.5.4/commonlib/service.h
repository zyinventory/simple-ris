#pragma once

extern char SERVICE_NAME[];

void ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void SvcReportEvent(LPCTSTR szFunction, WORD eventType = EVENTLOG_ERROR_TYPE, DWORD eventId = SVC_ERROR);
bool WINAPI SvcInit(DWORD dwWaitHint);
char *getServiceName();

