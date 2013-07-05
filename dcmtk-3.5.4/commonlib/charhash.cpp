#include "stdafx.h"

using namespace std;

static char char32map[32] = {'0', '1', '2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','J','K','M','N','P','Q','R','S','T','U','V','W','X','Y'};
static unsigned char inv32map[] = {
// 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 32, 32, 32, 32, 32, 32,  // 0x3?
  32, 10, 11, 12, 13, 14, 15, 16, 17, 32, 18, 19, 32, 20, 21, 32,  // 0x4?
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31                           // 0x5?
};

size_t toWchar(string &src, wchar_t **dest)
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

int toUTF8(wchar_t *pWide, string &dest)
{
  int nBytes = ::WideCharToMultiByte(CP_UTF8, 0, pWide, -1, NULL, 0, NULL, NULL);
  if(nBytes == 0) return 0;
  char* pNarrow = new char[nBytes + 1];
  nBytes = ::WideCharToMultiByte(CP_UTF8, 0, pWide, -1, pNarrow, nBytes + 1, NULL, NULL);
  dest = pNarrow;
  delete [] pNarrow;
  return nBytes;
}

int hashCodeW(const wchar_t *s, unsigned int seed)
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

int hashCode(const char *s, unsigned int seed)
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

bool encodeBase32(string &src, string &enc)
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

bool decodeBase32(string &src, string &dec)
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
