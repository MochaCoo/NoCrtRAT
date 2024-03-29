#include "Utility.h"
#include "number.h"

//#define NDEBUG
#include <assert.h>

size_t MyStrLenW(const wchar_t* s) {//长度不含终止符
	const wchar_t* o = s;
	while (*s) {
		s++;
	}
	return s - o;
}

size_t MyStrLen(const char* s) {//长度不含终止符
	const char* o = s;
	while (*s) {
		s++;
	}
	return s - o;
}

bool MyStrCmpW(const wchar_t* str1, const wchar_t* str2) {
	while (*str1 && *str2) {
		if (*str1 != *str2)
			return false;
		++str1; ++str2;
	}
	return *str1 == *str2;
}

void MyStrAddW(wchar_t* d, const wchar_t* s) {
	while (*d)
		d++;
	while (*d = *s) {
		d++; s++;
	}
}

void MyStrCpyW(wchar_t* d, const wchar_t* s) {
	while (*d = *s) {
		d++; s++;
	}
}

//wait:
//FAIL + state
//SUCCESS + string
//do not wait:
//FAIL + state
//SUCCESS
bool PipeCmd(wchar_t* pszCmd, wchar_t* CurrentDirectory, MySocket* pszResultBuffer, bool wait, DWORD EchoTimeout/*回显的最长等待时间(s)*/) {
	const DWORD codebase = 10;
	DWORD state;
	BOOL bRet;
	bool res = true;
	const DWORD sleeptime = 1;//0-1000

	void* buf = NULL;
	DWORD totalSize = 0;
	ULONGLONG time = 0;

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_HIDE;
	PROCESS_INFORMATION pi = { 0 };

	HANDLE hReadPipe = NULL;
	HANDLE hWritePipe = NULL;
	SECURITY_ATTRIBUTES securityAttributes = { 0 };

	if (wait) {
		// 设定管道的安全属性
		securityAttributes.bInheritHandle = TRUE;
		securityAttributes.nLength = sizeof(securityAttributes);
		securityAttributes.lpSecurityDescriptor = NULL;

		// 创建匿名管道
		bRet = CreatePipe(&hReadPipe, &hWritePipe, &securityAttributes, 0);
		if (!bRet)
		{
			pszResultBuffer->WriteUint32(FAIL);
			pszResultBuffer->WriteUint32(codebase + 1);
			return false;
		}

		// 设置新进程参数
		si.hStdError = hWritePipe;
		si.hStdOutput = hWritePipe;
		// 创建新进程执行命令, 将执行结果写入匿名管道中，子进程必须可继承父进程句柄
		bRet = CreateProcessW(NULL, pszCmd, NULL, NULL, TRUE, 0, NULL, CurrentDirectory, &si, &pi);
		if (!bRet)
		{
			pszResultBuffer->WriteUint32(FAIL);
			pszResultBuffer->WriteUint32(codebase + 2);
			return false;
		}
		DWORD size;
		time = GetTickCount64();//毫秒
		while (true) {
			if (PeekNamedPipe(hReadPipe, NULL, NULL, NULL, &size, NULL)) {
				if (size > 0) {
					totalSize += size;

					void* t = MyReAlloc(buf, totalSize + sizeof(char));
					if (t == NULL) {
						state = codebase + 3;
						goto fail;
					}
					buf = t;

					if (!ReadFile(hReadPipe, (char*)buf + totalSize - size, size, NULL, NULL)) {
						state = codebase + 4;
						goto fail;
					}
				}
				else {
					DWORD ExitCode;
					GetExitCodeProcess(pi.hProcess, &ExitCode);
					if (ExitCode != STILL_ACTIVE) {
						pszResultBuffer->WriteUint32(SUCCESS);
						if (buf == NULL) {
							pszResultBuffer->WriteString(L"** no echo **");
						}
						else {
							((char*)buf)[totalSize] = '\0';
							if (!ANSI2Unicode((char*)buf, pszResultBuffer)) {
								state = codebase + 5;
								goto fail;
							}
						}
						goto clean;
					}
				}
			}
			else {
				state = codebase + 6;
				goto fail;
			}
			//检查是否超时
			if (EchoTimeout != 0 && (GetTickCount64() - time) / 1000 > EchoTimeout) {
				TerminateProcess(pi.hProcess, 0);
				state = codebase + 7;
				goto fail;
			}
			Sleep(sleeptime);
		}
	fail:
		res = false;
		pszResultBuffer->Reset();
		pszResultBuffer->WriteUint32(FAIL);
		pszResultBuffer->WriteUint32(state);
	clean:
		CloseHandle(hWritePipe);
		CloseHandle(hReadPipe);
		MyFree(buf);
	clean1:
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return res;
	}
	else {//do not wait
		bRet = CreateProcessW(NULL, pszCmd, NULL, NULL, FALSE, 0, NULL, CurrentDirectory, &si, &pi);
		if (!bRet)
		{
			pszResultBuffer->WriteUint32(FAIL);
			state = codebase + 8;
			pszResultBuffer->WriteUint32(state);
			return false;
		}
		pszResultBuffer->WriteUint32(SUCCESS);
		//pszResultBuffer->WriteString(L"** do not wait **");
		goto clean1;
	}
}

bool ANSI2Unicode(const char* ansi,MySocket* s)
{
	int len;
	len = MultiByteToWideChar(CP_ACP, 0, ansi, -1, NULL, 0);
	wchar_t* szUtf16 = (wchar_t*)s->AllocBytes(len * sizeof(wchar_t));//包括终止符
	if (szUtf16 == NULL)
		return false;
	//wchar_t* szUtf16 = (wchar_t*)MyAlloc(len * sizeof(wchar_t));
	//memset(szUtf16, 0, len + 1);
	return MultiByteToWideChar(CP_ACP, 0, ansi, -1, szUtf16, len) != 0;
}