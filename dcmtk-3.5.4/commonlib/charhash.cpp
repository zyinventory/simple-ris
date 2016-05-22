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

static size_t toWchar(string &src, wchar_t **dest)
{
  size_t blen = src.length();
  if(blen == 0) { *dest = NULL; return 0; }

  size_t charlen = 0;
  errno_t en = mbstowcs_s(&charlen, NULL, 0, src.c_str(), blen);
  if(en != 0 || charlen <= 0) { *dest = NULL; return 0; }

  *dest = new wchar_t[charlen];
  size_t wlen = 0;
  mbstowcs_s(&wlen, *dest, charlen, src.c_str(), charlen);
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
  string src(s);
  wchar_t *p = NULL;
  size_t wlen = toWchar(src, &p);
  if(wlen == 0) return 0;
  unsigned int hash = hashCodeW(p, seed);
  delete p;
  return hash;
}

static __int64 uidHashImpl(__int64 hash, int hash131, char *buffer, size_t buffer_size)
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

COMMONLIB_API __int64 uidHashW(const wchar_t *s, char *buffer, size_t buffer_size)
{
	__int64 hash = hashCodeW(s, 31);
	int hash131 = hashCodeW(s, 131);
	return uidHashImpl(hash, hash131, buffer, buffer_size);
}

COMMONLIB_API __int64 uidHash(const char *s, char *buffer, size_t buffer_size)
{
	__int64 hash = hashCode(s, 31);
	int hash131 = hashCode(s, 131);
	return uidHashImpl(hash, hash131, buffer, buffer_size);
}

COMMONLIB_API errno_t SeriesInstancePath(const char *series, const string &instance, char *outputBuffer, size_t bufLen, char pathSeparator)
{
	uidHash(series, outputBuffer, bufLen);
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

static bool encodeBase32(string &src, string &enc)
{
  wchar_t *dest = NULL;
  size_t wlen = toWchar(src, &dest);
  if(wlen == 0) return false;

  //cout.setf(ios_base::hex, ios_base::basefield);
  stringstream encstr;
  unsigned short remain = 0;
  size_t rbit = 0;
  for(size_t i = 0; i < wlen; i++)
  {
	unsigned short c = dest[i];
	//cout << setw(4) << setfill('0') << c;
	switch(rbit)
	{
	case 1:
	  encstr << char32map[(c >> 12) | remain] << char32map[(c & 0xF80) >> 7] << char32map[(c & 0x7C) >> 2];
	  rbit = 2;
	  remain = (c & 0x3) << 3;
	  break;
	case 2:
	  encstr << char32map[(c >> 13) | remain] << char32map[(c & 0x1F00) >> 8] << char32map[(c & 0xF8) >> 3];
	  rbit = 3;
	  remain = (c & 0x7) << 2;
	  break;
	case 3:
	  encstr << char32map[(c >> 14) | remain] << char32map[(c & 0x3E00) >> 9] << char32map[(c & 0x1F0) >> 4];
	  rbit = 4;
	  remain = (c & 0xF) << 1;
	  break;
	case 4:
	  encstr << char32map[(c >> 15) | remain] << char32map[(c & 0x7C00) >> 10] << char32map[(c & 0x3E0) >> 5] << char32map[c & 0x1F];
	  rbit = 0;
	  remain = 0;
	  break;
	default:  //0
	  encstr << char32map[c >> 11] << char32map[(c & 0x7C0) >> 6] << char32map[(c & 0x3E) >> 1];
	  rbit = 1;
	  remain = (c & 0x1) << 4;
	  break;
	}
  }
  if(rbit > 0) encstr << char32map[remain];
  delete dest;
  enc = encstr.str();
  /*
  cout << enc;
  cout.unsetf(ios_base::hex);
  cout << endl;
  */
  return true;
}

static bool decodeBase32(string &src, string &dec)
{
  vector<wchar_t> decvt;
  string::size_type blen = src.length();
  if(blen > 0)
  {
	wchar_t remain = 0;
	size_t rbit = 0;
	for(size_t i = 0; i < blen; i++)
	{
	  if(src[i] < 0x30 || src[i] > 0x59) return false;
	  unsigned short c = inv32map[src[i] - 0x30];
	  if(c == 32) return false;
	  switch(rbit)
	  {
	  case 0:
		remain = c << 11;
		rbit = 5;
		break;
	  case 1:
		remain |= c << 10;
		rbit = 6;
		break;
	  case 2:
		remain |= c << 9;
		rbit = 7;
		break;
	  case 3:
		remain |= c << 8;
		rbit = 8;
		break;
	  case 4:
		remain |= c << 7;
		rbit = 9;
		break;
	  case 5:
		remain |= c << 6;
		rbit = 10;
		break;
	  case 6:
		remain |= c << 5;
		rbit = 11;
		break;
	  case 7:
		remain |= c << 4;
		rbit = 12;
		break;
	  case 8:
		remain |= c << 3;
		rbit = 13;
		break;
	  case 9:
		remain |= c << 2;
		rbit = 14;
		break;
	  case 10:
		remain |= c << 1;
		rbit = 15;
		break;
	  case 11:
		remain |= c;
		decvt.push_back(remain);
		rbit = 0;
		break;
	  case 12:
		remain |= c >> 1;
		decvt.push_back(remain);
		remain = c & 0x1;
		remain <<= 15;
		rbit = 1;
		break;
	  case 13:
		remain |= c >> 2;
		decvt.push_back(remain);
		remain = c & 0x3;
		remain <<= 14;
		rbit = 2;
		break;
	  case 14:
		remain |= c >> 3;
		decvt.push_back(remain);
		remain = c & 0x7;
		remain <<= 13;
		rbit = 3;
		break;
	  case 15:
		remain |= c >> 4;
		decvt.push_back(remain);
		remain = c & 0xF;
		remain <<= 12;
		rbit = 4;
		break;
	  default:
		return false;
	  }
	}
	if(rbit > 0) decvt.push_back(remain);
	decvt.push_back(L'\0'); // end of string
	size_t charlen = decvt.size();
	wchar_t *wsrc = new wchar_t[charlen];
	copy(decvt.begin(), decvt.end(), stdext::checked_array_iterator<wchar_t*>(wsrc, charlen));
	size_t bytelen = 0;
	errno_t en = wcstombs_s(&bytelen, NULL, 0, wsrc, 0);
	if(en != 0)
	{
	  delete[] wsrc;
	  return false;
	}
	char *dest = new char[bytelen];
	size_t count = 0;
	wcstombs_s(&count, dest, bytelen, wsrc, bytelen);
	dec = dest;
	delete[] dest;
	delete[] wsrc;
  }
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
