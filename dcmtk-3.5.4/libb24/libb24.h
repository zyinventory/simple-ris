#pragma once
				   //          11111111112222
                   //012345678901234567890123
const char base24code[] = "9TF7VMWB8XJ2CKRQ346YPDGH";
const char base24decode[40] = {						   // 0x32
      11,16,17,-1,18, 3, 8, 0,-1,-1,-1,-1,-1,-1,   //    2 3 4 5 6 7 8 9 _ _ _ _ _ _
-1,-1, 7,12,21,-1, 2,22,23,-1,10,13,-1, 5,-1,-1,   //_ _ B C D _ F G H _ J K _ M _ _
20,15,14,-1, 1,-1, 4, 6, 9,19 };                   //P Q R _ T _ V W X Y

typedef long (__stdcall *LOCK_FUNC_PTR)(long);
extern "C" int decodeCharge(const char *b24buf, unsigned int serial, LOCK_FUNC_PTR lockfunc);

#define TOTAL_BUY 992
#define MAX_BOX 20
#define CHARGE_BASE 1
#define COUNTER_SECTION 31
