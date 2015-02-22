#include "stdafx.h"
#include <io.h>
#include <commonlib.h>
#import <msxml3.dll>

using namespace std;

extern bool opt_verbose;

bool deleteDayStudy(const char *dayxml)
{
	if(opt_verbose) time_header_out(cout) << "start cleaning day " << dayxml << endl;
	MSXML2::IXMLDOMDocumentPtr dayIndex;
	dayIndex.CreateInstance(__uuidof(MSXML2::DOMDocument30));
	if(VARIANT_FALSE == dayIndex->load(dayxml)) return false;
	MSXML2::IXMLDOMNodeListPtr listptr = dayIndex->selectNodes("/Collection/Study");
	bool allOK = true;
	char studyPath[MAX_PATH];
	while(MSXML2::IXMLDOMNodePtr newStudy = listptr->nextNode())
	{
		_bstr_t studyUid = newStudy->Gettext();
		char hashBuf[9];
		__int64 hashStudy = uidHashW((LPCWSTR)studyUid, hashBuf, sizeof(hashBuf));
		sprintf_s(studyPath, MAX_PATH, "archdir\\%c%c\\%c%c\\%c%c\\%c%c\\%s",
			hashBuf[0], hashBuf[1], hashBuf[2], hashBuf[3], hashBuf[4], hashBuf[5], hashBuf[6], hashBuf[7], (LPCSTR)studyUid);
		if(deleteTree(studyPath, &cerr))
		{
			if(opt_verbose) time_header_out(cout) << studyPath << " delete OK" << endl;
		}
		else
		{
			time_header_out(cerr) << studyPath << " delete failed" << endl;
			allOK = false;
		}
	}
	return allOK;
}

static HANDLE hRead = INVALID_HANDLE_VALUE, hWriteOut = INVALID_HANDLE_VALUE, hWriteErr = INVALID_HANDLE_VALUE;
static DWORD WINAPI service_stderr_thread(LPVOID lpParam) 
{
	char buffer[256];
	DWORD byteRead = 0;
	ofstream *plog = (ofstream*)lpParam;
	while(ReadFile(hRead, buffer, sizeof(buffer), &byteRead, NULL))
	{
		plog->write(buffer, byteRead);
	}
	return 0;
}

static HANDLE hThread = NULL, hOldStdout = INVALID_HANDLE_VALUE, hOldStderr = INVALID_HANDLE_VALUE;
static int fderr = -1, fdout = -1;
static FILE oldout, olderr, *fpout = NULL, *fperr = NULL;
//this function must link in EXE, faild at _open_osfhandle(...) in DLL, reason unknown.
bool __stdcall captureStdoutToLogStream(std::ostream &flog)
{
	if(!flog.good()) return false;

	if (CreatePipe(&hRead, &hWriteOut, NULL, 0)
		&& DuplicateHandle(GetCurrentProcess(), hWriteOut, GetCurrentProcess(), &hWriteErr, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		hOldStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		hOldStderr = GetStdHandle(STD_ERROR_HANDLE);
		if(!SetStdHandle(STD_ERROR_HANDLE, hWriteErr))
		{
			flog << "replace STD_ERROR_HANDLE failed" << endl;
			return false;
		}
		if(!SetStdHandle(STD_OUTPUT_HANDLE, hWriteOut))
		{
			flog << "replace STD_OUTPUT_HANDLE failed" << endl;
			return false;
		}
		
		//_open_osfhandle get C runtime low level file descriptor
		fderr = _open_osfhandle(STD_ERROR_HANDLE, _O_WRONLY | _O_BINARY);
		if(fderr == -1)
		{
			flog << "_open_osfhandle(STD_ERROR_HANDLE) failed" << endl;
			return false;
		}

		fdout = _open_osfhandle(STD_OUTPUT_HANDLE, _O_WRONLY | _O_BINARY);
		if(fdout == -1)
		{
			flog << "_open_osfhandle(STD_OUTPUT_HANDLE) failed" << endl;
			return false;
		}
		/* skip replacing C runtime low level file descriptor
		if(_dup2(fdout, 1))
			flog << "_dup2(fdout, 1) error: " << errno << endl;
		else
			flog << "_dup2(fdout, 1) OK" << endl;

		if(_dup2(fderr, 2))
			flog << "_dup2(fderr, 2) error: " << errno << endl;
		else
			flog << "_dup2(fderr, 2) OK" << endl;
		*/

		//_fdopen create new FILE*, replace stdout and stderr.
		fperr = _fdopen(fderr, "wcb");
		if(fperr)
		{
			//memcpy(stderr, fperr, sizeof(FILE));
			olderr = *stderr;
			*stderr = *fperr;
		}
		else
		{
			flog << "_fdopen(fderr, \"wcb\") failed" << endl;
			return false;
		}

		fpout = _fdopen(fdout, "wcb");
		if(fpout)
		{
			//memcpy(stdout, fpout, sizeof(FILE));
			oldout = *stdout;
			*stdout = *fpout;
		}
		else
		{
			flog << "_fdopen(fpout, \"wcb\") failed" << endl;
			return false;
		}

		hThread = CreateThread(NULL, 0, service_stderr_thread, (LPVOID)&flog, 0, NULL);
		if(hThread)
			return true;
		else
			flog << "create service log thread failed" << endl;
	}
	else
		flog << "create pipe failed" << endl;
	return false;
}

//_close fdout and fderr, _close will close hWriteOut and hWriteErr.
void __stdcall releaseStdout(std::ostream &flog)
{
	*stderr = olderr;
	*stdout = oldout;
	int errClose = 1, outClose = 1;
	if(fpout) outClose = fclose(fpout);
	if(hOldStdout != INVALID_HANDLE_VALUE)
	{
		if(!SetStdHandle(STD_OUTPUT_HANDLE, hOldStdout))
			flog << "restore old STD_OUTPUT_HANDLE failed" << endl;
	}

	if(fperr) errClose = fclose(fperr);
	if(hOldStderr != INVALID_HANDLE_VALUE)
	{
		if(!SetStdHandle(STD_ERROR_HANDLE, hOldStderr))
			flog << "restore old STD_ERROR_HANDLE failed" << endl;
	}

	if(hThread)
	{
		if(WAIT_OBJECT_0 != WaitForSingleObject(hThread, 10 * 1000))
		{
			flog << "wait service log thread stop timeout" << endl;
			TerminateThread(hThread, -1);
		}
	}
	CloseHandle(hRead);

	if(opt_verbose) flog << "releaseStdout: outClose = " << outClose << ", errClose = " << errClose << endl;
}
