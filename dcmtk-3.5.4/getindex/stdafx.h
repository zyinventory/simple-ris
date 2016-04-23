// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>


#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <atlbase.h>
#include <atlstr.h>

#if defined(_DEBUG)
#include <crtdbg.h>
#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif // DEBUG_NEW
#endif // _DEBUG

// TODO: 在此处引用程序需要的其他头文件
#include <fstream>
#include <sstream>
#include <list>
#include <algorithm>
#include <commonlib.h>
#include <study_struct.h>
#include <dcmdynamic.h>
extern "C" {
#include <cgic.h>
}

extern bool opt_verbose, errflag;
extern std::iostream *perrlog;
#define index_errlog (*perrlog)
void outputContent();
size_t collectionToFileNameList(const char *xmlpath, std::list<Study> &filenames, bool isPatient);
