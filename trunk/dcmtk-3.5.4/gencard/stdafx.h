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

// TODO: 在此处引用程序需要的其他头文件
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <direct.h>

class numpunct_no_gouping : public std::numpunct_byname<char>
{
public:
    numpunct_no_gouping(const char* name) : std::numpunct_byname<char>(name){ }
protected:
    virtual std::string do_grouping() const { return TEXT(""); } // no grouping
};
