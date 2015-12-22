#include "stdafx.h"

void call_process_log(const std::string &sessionId)
{
    return process_log(sessionId.c_str());
}
