#pragma once

#include <Windows.h>
#include "MyPacket.h"

#define MyMemCpy(d,s,l) for(size_t mci=0;mci<(l);mci++){*((char*)(d)+mci)=*((char*)(s)+mci);}
size_t MyStrLenW(const wchar_t* s);
size_t MyStrLen(const char* s);
bool MyStrCmpW(const wchar_t* str1, const wchar_t* str2);
void MyStrAddW(wchar_t* d, const wchar_t* s);
void MyStrCpyW(wchar_t* d, const wchar_t* s);
bool ANSI2Unicode(const char* ansi, MySocket * s);

#define MyAlloc(size) HeapAlloc(GetProcessHeap(), 0, (size_t)(size))
#define MyReAlloc(p,size) ((p) == NULL ? MyAlloc(size) : HeapReAlloc(GetProcessHeap(), 0, (void*)(p), (size_t)(size)))
#define MyFree(p) HeapFree(GetProcessHeap(), 0, (void*)(p))
#define MyMemSize(p) HeapSize(GetProcessHeap(), 0, (void*)(p))//¿ÉÒÔ¿ÕÖ¸Õë
#define MyCreateThread(lpStartAddress,lpParameter) CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)lpStartAddress,(LPVOID)lpParameter,0,NULL)

bool PipeCmd(wchar_t* pszCmd, wchar_t* CurrentDirectory, MySocket * pszResultBuffer, bool wait, DWORD EchoTimeout);
