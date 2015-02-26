#include <windows.h>
#include <direct.h>        /* for _mkdir() */
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#define PATH_SEPARATOR '\\'
using namespace std;

#ifdef COMMONLIB_EXPORTS
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
static
#endif
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

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API __int64 uidHashW(const wchar_t *s, char *buffer, size_t buffer_size)
#else
__int64 uidHashW_internal(const wchar_t *s, char *buffer, size_t buffer_size)
#endif
{
	__int64 hash = hashCodeW(s, 31);
	int hash131 = hashCodeW(s, 131);
	return uidHashImpl(hash, hash131, buffer, buffer_size);
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API __int64 uidHash(const char *s, char *buffer, size_t buffer_size)
#else
__int64 uidHash_internal(const char *s, char *buffer, size_t buffer_size)
#endif
{
	__int64 hash = hashCode(s, 31);
	int hash131 = hashCode(s, 131);
	return uidHashImpl(hash, hash131, buffer, buffer_size);
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API bool MkdirRecursive(const char *subdir)
#else
static bool MkdirRecursive_internal(const char *subdir)
#endif
{
	// check if the subdirectory is already existent
	if( _mkdir(subdir) )
	{
		if(errno == EEXIST) return true;
	}
	else
	{
		return true;
	}

	string subdirectoryPath = subdir;
	size_t position = subdirectoryPath.rfind(PATH_SEPARATOR);
	if(position != string::npos)
	{
		string upperLevel = subdirectoryPath.substr(0, position);
#ifdef COMMONLIB_EXPORTS
		bool mkResult = MkdirRecursive(upperLevel.c_str());
#else
		bool mkResult = MkdirRecursive_internal(upperLevel.c_str());
#endif
		if(mkResult != true)
		{
			return mkResult;
		}
		// else: upper level exist, create current level
	}

	// if it is not existent create it
	if( _mkdir( subdirectoryPath.c_str() ) == -1 && errno != EEXIST)
	{
		cerr << "Could not create subdirectory " << subdirectoryPath.c_str() << endl;
		return false;
	}
	else
	{
		return true;
	}
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API bool prepareFileDir(const char *path)
#else
bool prepareFileDir_internal(const char *path)
#endif
{
	string filePath = path;
	string::size_type p = filePath.rfind(PATH_SEPARATOR);
	if(p == string::npos || p == 0) return true;
#ifdef COMMONLIB_EXPORTS
	return MkdirRecursive(filePath.substr(0, p).c_str());
#else
	return MkdirRecursive_internal(filePath.substr(0, p).c_str());
#endif
}

#ifdef COMMONLIB_EXPORTS
COMMONLIB_API int changeWorkingDirectory(int argc, char **argv, char *pPacsBase)
#else
int changeWorkingDirectory_internal(int argc, char **argv, char *pPacsBase)
#endif
{
	if(argc > 0)
	{
		char **endPos = argv + argc;
		char **pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "-wd")));
		if(pos == endPos) pos = find_if(argv, endPos, not1(bind1st(ptr_fun(strcmp), "--working-directory")));
		if(pos != endPos)
		{
			if(! _chdir(*++pos))
			{
				if(pPacsBase) strcpy_s(pPacsBase, MAX_PATH, *++pos);
				return -2;
			}
			else
				return 0;
		}
		// else get working dir from PACS_BASE in env
	}

	char* workingDirBuffer;
	size_t requiredSize;
	getenv_s( &requiredSize, NULL, 0, "PACS_BASE");
	if(requiredSize > 0)
	{
		char pacsdir[] = "\\pacs";
		workingDirBuffer = new char[requiredSize + sizeof(pacsdir)];
		getenv_s( &requiredSize, workingDirBuffer, requiredSize, "PACS_BASE");
		if(pPacsBase)
			strcpy_s(pPacsBase, MAX_PATH, workingDirBuffer);
		strcpy_s(workingDirBuffer + requiredSize - 1, sizeof(pacsdir), pacsdir);
		int chdirFail = _chdir(workingDirBuffer);
		delete workingDirBuffer;
		if(chdirFail) return -1;
	}
	else
	{
		if(!_chdir("C:\\usr\\local\\dicom\\pacs"))
			return -3;
		if(pPacsBase)
		{
			char base[] = "C:\\usr\\local\\dicom";
			strcpy_s(pPacsBase, sizeof(base), base);
		}
	}
	return 0;
}
