#pragma once

extern _declspec(dllimport) char service_name[];

void __stdcall ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void __stdcall SvcReportEvent(LPCTSTR szFunction, WORD eventType = EVENTLOG_ERROR_TYPE, DWORD eventId = SVC_ERROR);
bool __stdcall SvcInit(DWORD dwWaitHint);
//bool __stdcall captureStdoutToLogStream(std::ostream &flog);
//void __stdcall releaseStdout(std::ostream &flog);
