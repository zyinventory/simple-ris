#pragma once
const unsigned long key1 = 54762, key2 = 52662, key3 = 58962, key4 = 40062;
static char init_passwd[9] = "l2IH3.m/"; // abcdefgh
extern "C" unsigned long __stdcall ReadLock(int, unsigned char*, char*);
extern "C" unsigned long __stdcall Lock32_Function(unsigned long);
extern "C" int __stdcall WriteLock(int, unsigned char*, char*);
extern "C" int __stdcall Counter(char*);
extern "C" int __stdcall SetLock(int, unsigned long*, char*, char*);
extern "C" int __stdcall InOutMessageBox(char*);
extern "C" void __stdcall UnShieldLock();
