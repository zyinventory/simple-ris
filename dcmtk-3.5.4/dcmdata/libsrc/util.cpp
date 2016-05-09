#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>
#include <process.h>

static _timeb current_sequence_val = {0LL, 0, 0, 0};
static __time64_t current_sequence_val_diff;

size_t in_process_sequence(char *buff, size_t buff_size, const char *prefix)
{
    if(strcpy_s(buff, buff_size, prefix)) return 0;
    size_t buff_used = strlen(buff);

    struct _timeb storeTimeThis;
    struct tm localtime;
    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < current_sequence_val.time || (storeTimeThis.time == current_sequence_val.time && storeTimeThis.millitm <= current_sequence_val.millitm))
    {
        if(current_sequence_val.millitm == 999)
        {
            ++current_sequence_val.time;
            current_sequence_val.millitm = 0;
        }
        else
            ++current_sequence_val.millitm;

        current_sequence_val_diff = (current_sequence_val.time - storeTimeThis.time) * 1000 + current_sequence_val.millitm - storeTimeThis.millitm;
        storeTimeThis = current_sequence_val;
    }
    else
    {
        current_sequence_val = storeTimeThis;
        current_sequence_val_diff = 0;
    }

    localtime_s(&localtime, &storeTimeThis.time);
    size_t sf_size = strftime(buff + buff_used, buff_size - buff_used, "%Y%m%d%H%M%S", &localtime);
    if(sf_size == 0) return 0;
    buff_used += sf_size;
    int sp_size = sprintf_s(buff + buff_used, buff_size - buff_used, "_%03hd-%lld-%x", 
        storeTimeThis.millitm, current_sequence_val_diff, _getpid());
    if(sp_size == -1) return 0;
    buff_used += sp_size;
    return buff_used;
}
