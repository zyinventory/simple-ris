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

// TODO: �ڴ˴����ó�����Ҫ������ͷ�ļ�
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
