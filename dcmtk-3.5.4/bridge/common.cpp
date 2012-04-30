#include "stdafx.h"

bool IsASCII(const char *str)
{
  bool isAscii = true;
  if(str == NULL) return isAscii;
  size_t length = strlen(str);
  for(unsigned int i = 0; i < length; i++)
  {
    if(str[i] > '~')
    {
      isAscii = false;
      break;
    }
  }
  return isAscii;
}
