#include <stdio.h>
#include <io.h>
#define TEMP_FILE_NAME_SIZE 10

#ifdef _WIN32
#include <windows.h>
static void PrintError(DWORD syserror)
{
		LPSTR pbuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, syserror, 0, (LPSTR)&pbuf, 0, NULL);
		fputs(pbuf, stderr);
		fputc('\n', stderr);
		if(pbuf) LocalFree(pbuf);
}

static HANDLE CreateTempFile()
{
	size_t requiredSize = 0;
	getenv_s(&requiredSize, NULL, 0, "TMP");
	char *temp_file_name = NULL;
	if(requiredSize == 0)
	{
		requiredSize = 2;
		temp_file_name = new char[requiredSize + TEMP_FILE_NAME_SIZE];
		temp_file_name[0] = '.';
	}
	else
	{
		temp_file_name = new char[requiredSize + TEMP_FILE_NAME_SIZE];
		getenv_s(&requiredSize, temp_file_name, requiredSize, "TMP");
	}
	tmpnam_s(temp_file_name + requiredSize - 1, TEMP_FILE_NAME_SIZE);
	HANDLE tfh = CreateFile(temp_file_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
		CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if(tfh == INVALID_HANDLE_VALUE) PrintError(GetLastError());
	if(temp_file_name)
	{
		//fprintf(stderr, "temp file is %s\n", temp_file_name);
		delete temp_file_name;
	}
	return tfh;
}

static char **ppOut;
static size_t *pSize;
FILE *open_memstream(char **l_out, size_t *l_size)
{
	ppOut = NULL;
	pSize = NULL;
	HANDLE tfh = CreateTempFile();
	if(tfh == INVALID_HANDLE_VALUE) return NULL;
	int fd = _open_osfhandle((intptr_t)tfh, 0);
	if(fd == -1)
	{
		fputs("can't convert HANDLE to FD\n", stderr);
		CloseHandle(tfh);
		return NULL;
	}
	FILE *fp = _fdopen(fd, "r+b");
	if(fp == NULL)
	{
		fputs("can't convert FD to FILE*\n", stderr);
		if(fd != -1 && fd != 0 && fd != 1 && fd != 2) _close(fd);
		return NULL;
	}
	ppOut = l_out;
	pSize = l_size;
	return fp;
}

int get_buffer_and_size(FILE *fp)
{
	if(fp == NULL) return -1;
	int result = fseek(fp, 0, SEEK_END);
	if(result) return result;
	long pos = ftell(fp);
	if(pos <= 0) return -3;
	fseek(fp, 0, SEEK_SET);
	char *pOut = new char[pos];
	size_t readFromFile = fread(pOut, 1, pos, fp);
	if(readFromFile == pos)
	{
		*pSize = pos;
		*ppOut = pOut;
		return 0;
	}
	else
	{
		*pSize = readFromFile;
		delete pOut;
		return 1;
	}
}
#endif //_WIN32
