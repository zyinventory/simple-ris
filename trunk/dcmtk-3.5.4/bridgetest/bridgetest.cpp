// bridgetest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "bridge.h"

int _tmain(int argc, _TCHAR* argv[])
{
  char imageManageNumber[15];

  if(connectDicomDB())
  {
	  if(getManageNumber(imageManageNumber, NULL, 0))
    {
      printf(imageManageNumber);
      return 0;
    }
  }
  else
    return -1;
}

