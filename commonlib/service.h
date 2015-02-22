#pragma once

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#define SVC_ERROR ((DWORD)0xC0020001L)
#define SVC_INFO  ((DWORD)0x60020002L)

extern COMMONLIB_API char service_name[];

COMMONLIB_API void __stdcall ReportSvcStatus( DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
COMMONLIB_API void __stdcall SvcReportEvent(LPCTSTR szFunction, WORD eventType = EVENTLOG_ERROR_TYPE, DWORD eventId = SVC_ERROR);
COMMONLIB_API bool __stdcall SvcInit(DWORD dwWaitHint);
//bool __stdcall captureStdoutToLogStream(std::ostream &flog);
//void __stdcall releaseStdout(std::ostream &flog);
