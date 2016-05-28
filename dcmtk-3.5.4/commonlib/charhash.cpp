#include "stdafx.h"
#include "commonlib.h"
#include "UIDBase36.h"
using namespace std;

static char char32map[32] = {'0', '1', '2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','J','K','M','N','P','Q','R','S','T','U','V','W','X','Y'};
static unsigned char inv32map[] = {
// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 32, 32, 32, 32, 32, 32,  // 0x3?
  32, 10, 11, 12, 13, 14, 15, 16, 17, 32, 18, 19, 32, 20, 21, 32,  // 0x4?
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31                           // 0x5?
};

COMMONLIB_API bool IsASCII(const char *str)
{
  bool isAscii = true;
  if(str == NULL) return isAscii;
  size_t length = strlen(str);
  for(unsigned int i = 0; i < length; i++)
  {
    if(0 == isprint(str[i]))
    {
      isAscii = false;
      break;
    }
  }
  return isAscii;
}

COMMONLIB_API int UTF8ToGBK(const char *lpUTF8Str, char *lpGBKStr, int nGBKStrLen)
{
    wchar_t * lpUnicodeStr = NULL;
    int nRetLen = 0;

    //如果UTF8字符串为NULL则出错退出
    //输出缓冲区为空则返回转换后需要的空间大小
    if(lpUTF8Str == NULL || lpGBKStr == NULL || nGBKStrLen <= 0) return 0; 

    nRetLen = MultiByteToWideChar(CP_UTF8, 0, lpUTF8Str, -1, NULL, NULL);  //获取转换到Unicode编码后所需要的字符空间长度
    lpUnicodeStr = new wchar_t[nRetLen + 1];  //为Unicode字符串空间

    nRetLen = MultiByteToWideChar(CP_UTF8, 0, lpUTF8Str, -1, lpUnicodeStr, nRetLen);  //转换到Unicode编码
    if(nRetLen == 0) goto clean_allocated_memory; //转换失败则出错退出

    nRetLen = WideCharToMultiByte(936, 0, lpUnicodeStr, -1, NULL, NULL, NULL, NULL);  //获取转换到GBK编码后所需要的字符空间长度
    if(nGBKStrLen < nRetLen) goto clean_allocated_memory; //如果输出缓冲区长度不够则退出

    nRetLen = WideCharToMultiByte(936, 0, lpUnicodeStr, -1, lpGBKStr, nRetLen, NULL, NULL);  //转换到GBK编码

clean_allocated_memory:
    if(lpUnicodeStr) delete []lpUnicodeStr;
    return nRetLen;
}

COMMONLIB_API int GBKToUTF8(const char *lpGBKStr, char *lpUTF8Str, int nUTF8StrLen)
{
    wchar_t * lpUnicodeStr = NULL;
    int nRetLen = 0;

    //如果UTF8字符串为NULL则出错退出
    //输出缓冲区为空则返回转换后需要的空间大小
    if(lpUTF8Str == NULL || lpGBKStr == NULL || nUTF8StrLen <= 0) return 0; 

    nRetLen = MultiByteToWideChar(936, 0, lpGBKStr, -1, NULL, NULL);  //获取转换到Unicode编码后所需要的字符空间长度
    lpUnicodeStr = new wchar_t[nRetLen + 1];  //为Unicode字符串空间

    nRetLen = MultiByteToWideChar(936, 0, lpGBKStr, -1, lpUnicodeStr, nRetLen);  //转换到Unicode编码
    if(nRetLen == 0) goto clean_allocated_memory; //转换失败则出错退出

    nRetLen = WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, NULL, NULL, NULL, NULL);  //获取转换到GBK编码后所需要的字符空间长度
    if(nUTF8StrLen < nRetLen) goto clean_allocated_memory; //如果输出缓冲区长度不够则退出

    nRetLen = WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, lpUTF8Str, nRetLen, NULL, NULL);  //转换到GBK编码

clean_allocated_memory:
    if(lpUnicodeStr) delete []lpUnicodeStr;
    return nRetLen;
}

static size_t toWchar(const char *src, wchar_t **dest)
{
  size_t blen = strlen(src);
  if(blen == 0) { *dest = NULL; return 0; }

  size_t charlen = 0;
  errno_t en = mbstowcs_s(&charlen, NULL, 0, src, blen);
  if(en != 0 || charlen <= 0) { *dest = NULL; return 0; }

  *dest = new wchar_t[charlen];
  size_t wlen = 0;
  mbstowcs_s(&wlen, *dest, charlen, src, charlen);
  if(wlen > 0 && (*dest)[wlen - 1] == L'\0') --wlen;
  if(wlen == 0) { delete *dest; *dest = NULL; return 0; }
  return wlen;
}

static int toUTF8(wchar_t *pWide, string &dest)
{
  int nBytes = ::WideCharToMultiByte(CP_UTF8, 0, pWide, -1, NULL, 0, NULL, NULL);
  if(nBytes == 0) return 0;
  char* pNarrow = new char[nBytes + 1];
  nBytes = ::WideCharToMultiByte(CP_UTF8, 0, pWide, -1, pNarrow, nBytes + 1, NULL, NULL);
  dest = pNarrow;
  delete [] pNarrow;
  return nBytes;
}

static int hashCodeW(const wchar_t *s, unsigned int seed)
{
  size_t wlen = wcsnlen_s(s, 128);
  if(wlen == 0) return 0;
  unsigned int hash = 0;
  for(size_t i = 0; i < wlen; i++)
  {
	hash = hash * seed + s[i];
  }
  return hash;
}

static int hashCode(const char *s, unsigned int seed)
{
  if(s == NULL) return 0;
  wchar_t *p = NULL;
  size_t wlen = toWchar(s, &p);
  if(wlen == 0) return 0;
  unsigned int hash = hashCodeW(p, seed);
  delete p;
  return hash;
}

static __int64 HashStrImpl(__int64 hash, int hash131, char *buffer, size_t buffer_size)
{
	hash <<= 9;
	hash &= 0x1FFFFFFFFFFL;
	hash131 >>= 23;
	hash131 &= 0x1FF;
	hash |= hash131;
	if(buffer && ! _i64toa_s(hash, buffer, buffer_size, 36))
	{
		_strupr_s(buffer, buffer_size);
		size_t buflen = strlen(buffer);
		if(buflen < 8 && buffer_size >=9)
		{
			for(int i = 0; i < 9; ++i)
			{
				int tail = buflen - i;
				if(tail < 0)
					buffer[8 - i] = '0';
				else
					buffer[8 - i] = buffer[tail];
			}
		}
	}
	return hash;
}

COMMONLIB_API __int64 HashStrW(const wchar_t *s, char *buffer, size_t buffer_size)
{
	__int64 hash = hashCodeW(s, 31);
	int hash131 = hashCodeW(s, 131);
	return HashStrImpl(hash, hash131, buffer, buffer_size);
}

COMMONLIB_API __int64 HashStr(const char *s, char *buffer, size_t buffer_size)
{
	__int64 hash = hashCode(s, 31);
	int hash131 = hashCode(s, 131);
	return HashStrImpl(hash, hash131, buffer, buffer_size);
}

COMMONLIB_API errno_t SeriesInstancePath(const char *series, const string &instance, char *outputBuffer, size_t bufLen, char pathSeparator)
{
	HashStr(series, outputBuffer, bufLen);
	char compbuf[UIDBase36::COMPRESS_MAX_LEN + 1];
	size_t complen = UIDBase36::instance.compress(instance, compbuf, UIDBase36::COMPRESS_MAX_LEN + 1);
	const char *src;
	if(complen > UIDBase36::COMPRESS_LEN)
	{
		size_t overflow = complen - UIDBase36::COMPRESS_LEN;
		outputBuffer[7 - overflow] = '_';
		strncpy_s(&outputBuffer[1 + 7 - overflow], bufLen - (8 - overflow), compbuf, overflow);
		outputBuffer[8] = pathSeparator;
		src = &compbuf[overflow];
	}
	else
	{
		outputBuffer[8] = pathSeparator;
		src = compbuf;
	}

	for(int groupCount = 1; *src != '\0'; src += 8, ++groupCount)
	{
		outputBuffer[9 * groupCount - 1] = pathSeparator;
		strncpy_s(&outputBuffer[9 * groupCount], bufLen - 9 * groupCount, src, 8);
	}
	return 0;
}

COMMONLIB_API bool EncodeBase32(const char *src, char *enc, size_t enc_buf_size)
{
    size_t src_len = strlen(src);
    if(src_len == 0 || src_len > enc_buf_size) return false;
    size_t rbit = 0;
    unsigned char remain = 0;
    vector<char> encstr;
    for(size_t i = 0; i < src_len; ++i)
    {
        char c = src[i];
        switch(rbit)
        {
        case 0:
            encstr.push_back(char32map[c >> 3]);
            rbit = 3;
            remain = (c & 0x7) << 2;
            break;
        case 1:
            encstr.push_back(char32map[(c >> 4) | remain]);
            rbit = 4;
            remain = (c & 0xf) << 1;
            break;
        case 2:
            encstr.push_back(char32map[(c >> 5) | remain]);
            encstr.push_back(char32map[c & 0x1f]);
            rbit = 0;
            remain = 0;
            break;
        case 3:
            encstr.push_back(char32map[(c >> 6) | remain]);
            encstr.push_back(char32map[(c >> 1) & 0x1f]);
            rbit = 1;
            remain = (c & 0x1) << 4;
            break;
        case 4:
            encstr.push_back(char32map[(c >> 7) | remain]);
            encstr.push_back(char32map[(c >> 2) & 0x1f]);
            rbit = 2;
            remain = (c & 0x3) << 3;
            break;
        default:
            return false;
        }
    }
    if(rbit > 0) encstr.push_back(char32map[remain]);
    if(encstr.size() + 1 > enc_buf_size) return false;
    stdext::checked_array_iterator<char*> enc_end = copy(encstr.begin(), encstr.end(), stdext::checked_array_iterator<char*>(enc, enc_buf_size));
    *enc_end = '\0';
    return true;
}

COMMONLIB_API bool DecodeBase32(const char *src, char *dec, size_t dec_buf_size)
{
    vector<char> decvt;
    string::size_type src_len = strlen(src);
    if(src_len <= 0) return false;
    char remain = 0;
    size_t rbit = 0;
    for(size_t i = 0; i < src_len; ++i)
    {
        if(src[i] < 0x30 || src[i] > 0x59) return false;
        unsigned short c = inv32map[src[i] - 0x30];
        if(c == 32) return false;
        switch(rbit)
        {
        case 0:
            remain = c << 3;
            rbit = 5;
            break;
        case 1:
            remain |= c << 2;
            rbit = 6;
            break;
        case 2:
            remain |= c << 1;
            rbit = 7;
            break;
        case 3:
            remain |= c;
            decvt.push_back(remain);
            rbit = 0;
            remain = 0;
            break;
        case 4:
            remain |= c >> 1;
            decvt.push_back(remain);
            rbit = 1;
            remain = (c & 0x10) << 7;
            break;
        case 5:
            remain |= c >> 2;
            decvt.push_back(remain);
            rbit = 2;
            remain = (c & 0x3) << 6;
            break;
        case 6:
            remain |= c >> 3;
            decvt.push_back(remain);
            rbit = 3;
            remain = (c & 0x7) << 5;
            break;
        case 7:
            remain |= c >> 4;
            decvt.push_back(remain);
            rbit =4;
            remain = (c & 0xf) << 4;
            break;
        default:
            return false;
        }
    }
    if(decvt.size() + 1 > dec_buf_size) return false;
    stdext::checked_array_iterator<char*> dec_end = copy(decvt.begin(), decvt.end(), stdext::checked_array_iterator<char*>(dec, dec_buf_size));
    *dec_end = '\0';
    return true;
}

COMMONLIB_API size_t extractStudyUid(char *buffer, const size_t bufferSize, const wchar_t *body)
{
	const wchar_t *body2 = wcsstr(body, L"StudyInstanceUID");
	wregex pattern(L"StudyInstanceUID=\"([\\d\\.]+)\".*");
	wcmatch mr;
	size_t count = 0;

	if(regex_match(body2, mr, pattern, 
		regex_constants::match_flag_type::format_first_only | regex_constants::match_flag_type::format_no_copy))
	{
		//wcmatch::const_reference == const sub_match<const wchar_t*> &
		wcmatch::const_reference uidIt = *++mr.begin();
		wcstombs_s(&count, buffer, bufferSize, uidIt.first, uidIt.length());
	}
	return count;
}
