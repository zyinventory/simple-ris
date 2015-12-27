#include <time.h>
#include <sys/timeb.h>
#include <sstream>
#include <iomanip>

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API size_t GenerateTime(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#else
size_t GenerateTime_internal(const char *format, char *timeBuffer, size_t bufferSize, time_t *time_now)
#endif
{
	time_t t, *pt = time_now ? time_now : &t;
	*pt = time( NULL );
	struct tm tmp;
	if( localtime_s( &tmp, pt ) == 0 )
		return strftime(timeBuffer, bufferSize, format, &tmp);
	else
		return 0;
}

static struct _timeb storeTimeHistory = {0, 0, 0, 0};
#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int GetNextUniqueNo(const char *prefix, char *pbuf, const size_t buf_size)
#else
int GetNextUniqueNo_internal(const char *prefix, char *pbuf, const size_t buf_size)
#endif
{
    struct _timeb storeTimeThis;
    __time64_t diff = 0;
    if(buf_size < 40) return -1;

    _ftime_s(&storeTimeThis);
    if(storeTimeThis.time < storeTimeHistory.time || (storeTimeThis.time == storeTimeHistory.time && storeTimeThis.millitm <= storeTimeHistory.millitm))
    {
        if(storeTimeHistory.millitm == 999)
        {
            ++storeTimeHistory.time;
            storeTimeHistory.millitm = 0;
        }
        else
            ++storeTimeHistory.millitm;

        diff = (storeTimeHistory.time - storeTimeThis.time) * 1000 + storeTimeHistory.millitm - storeTimeThis.millitm;
        storeTimeThis = storeTimeHistory;
    }
    else
        storeTimeHistory = storeTimeThis;

    std::stringstream strmbuf;
    strmbuf << prefix << storeTimeThis.time << std::setw(3) << std::setfill('0') << storeTimeThis.millitm << "-" << diff;
    std::string s(strmbuf.str());
    strcpy_s(pbuf, buf_size, s.c_str());
    return s.length();
}
