#ifndef COMMON_PUBLIC_H
#define COMMON_PUBLIC_H

#define time_header_out time_header_out_internal
std::ostream& time_header_out_internal(std::ostream &os);
#define displayErrorToCerr displayErrorToCerr_internal
DWORD displayErrorToCerr_internal(const TCHAR *lpszFunction, DWORD dw, std::ostream *perrstrm = NULL);
#define GetSignalInterruptValue GetSignalInterruptValue_internal
int GetSignalInterruptValue_internal();
#define SignalInterruptHandler SignalInterruptHandler_internal
void SignalInterruptHandler_internal(int signal);
#define Capture_Ctrl_C Capture_Ctrl_C_internal
void Capture_Ctrl_C_internal();
#define GetPacsBase GetPacsBase_internal
const char* GetPacsBase_internal();
#define ChangeToPacsWebSub ChangeToPacsWebSub_internal
int ChangeToPacsWebSub_internal(char *pPacsBase, size_t buff_size);

#endif // COMMON_PUBLIC_H
