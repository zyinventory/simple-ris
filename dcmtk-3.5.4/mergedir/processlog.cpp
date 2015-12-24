#include "stdafx.h"
using namespace std;

void call_process_log(std::string &sessionId)
{
    process_log(sessionId.c_str(), false);
}
