#pragma once

extern _declspec(dllimport) char service_name[];
extern _declspec(dllimport) std::ostream * ptr_ostream_service_log;

void ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void SvcReportEvent(LPCTSTR szFunction, WORD eventType = EVENTLOG_ERROR_TYPE, DWORD eventId = SVC_ERROR);
bool WINAPI SvcInit(DWORD dwWaitHint);
