// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // ĳЩ CString ���캯��������ʽ��

#include <atlbase.h>
#include <atlstr.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif // DEBUG_NEW
#endif // _DEBUG

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
#include <fstream>
#include <sstream>
#include <list>
#include <commonlib.h>
#include <study_struct.h>
#include <dcmdynamic.h>
#include <SimpleIni.h>
extern "C" {
#include <cgic.h>
}

extern std::ostringstream index_errlog;
void outputContent(bool error);
size_t collectionToFileNameList(const char *xmlpath, std::list<Study> &filenames, bool isPatient);
