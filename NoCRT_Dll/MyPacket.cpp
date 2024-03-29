#include "MyPacket.h"
#include "MyDEBUG.h"
#include "Utility.h"

#pragma comment(lib, "ws2_32.lib")
#include <ws2tcpip.h>

bool MySocket::MySocketInit(HANDLE mu)
{
	this->pRecvBuff = NULL;
	this->RecvBodySize = 0;
	this->roff = 0;
	this->pSendBuff = NULL;
	this->SendBuffSize = 0;
	this->woff = 0;
	this->success = true;

	if (mu == NULL) {
		this->sock = INVALID_SOCKET;
		return false;
	}
	this->m = mu;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		this->sock = INVALID_SOCKET;
		return false;
	}

	this->sock = WSASocket(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_NO_HANDLE_INHERIT | WSA_FLAG_OVERLAPPED);
	if (this->sock == INVALID_SOCKET) {
		//WSAGetLastError()
		//freeaddrinfo(result);
		WSACleanup();
		return false;
	}
	return true;
}

MySocket* MySocket::New()//使用当前socket和互斥体创建一个新类
{
	MySocket* c = (MySocket*)MyAlloc(sizeof(MySocket));
	if (c == NULL)
		return NULL;
	c->pRecvBuff = NULL;
	c->RecvBodySize = 0;
	c->pSendBuff = NULL;
	c->SendBuffSize = 0;
	c->roff = 0;
	c->woff = 0;
	c->m = this->m;
	c->sock = this->sock;
	c->success = true;
	return c;
}

void MySocket::FreeCopy()
{
	this->sock = INVALID_SOCKET;
	MyFree(this->pRecvBuff);
	MyFree(this->pSendBuff);
	this->pRecvBuff = NULL;
	this->pSendBuff = NULL;
}

void MySocket::Free()
{
	WaitForSingleObject(this->m, INFINITE);
	if (this->sock != INVALID_SOCKET) {
		shutdown(this->sock, SD_BOTH);
		closesocket(this->sock);
		WSACleanup();
		this->FreeCopy();
		//this->sock = INVALID_SOCKET;
		//MyFree(this->pRecvBuff);
		//MyFree(this->pSendBuff);
		//this->pRecvBuff = NULL;
		//this->pSendBuff = NULL;
	}
	ReleaseMutex(this->m);
}

bool MySocket::Connect(const char* IP, u_short port)
{
	if (this->sock == INVALID_SOCKET) {
		if (!this->MySocketInit(this->m))
			return false;
	}
	struct  addrinfo        hints = { 0 };
	struct  addrinfo* ai_list = NULL;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	sockaddr_in sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_port = htons(port);

	if (getaddrinfo(IP, NULL, &hints, &ai_list) != 0) {
		inet_pton(AF_INET, IP, &sockAddr.sin_addr.s_addr);
	}
	else {
		sockAddr.sin_addr.s_addr = ((struct  sockaddr_in*)ai_list->ai_addr)->sin_addr.S_un.S_addr;
	}

	if (connect(this->sock, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		//closesocket(this->sock);
		return false;
	}
	return true;
}

bool MySocket::Prealloc(size_t l) {
	if (this->pSendBuff == NULL) {
		this->woff = sizeof(MySendPacketHead);
		this->SendBuffSize = sizeof(MySendPacketHead) + l;
		this->pSendBuff = MyAlloc(this->SendBuffSize);
		if (this->pSendBuff == NULL) {
			MyDebug("Prealloc: Alloc Memory failed");
			return false;
		}
	}
	else if (this->woff + l > this->SendBuffSize) {
		this->SendBuffSize = this->woff + l;
		void* t = MyReAlloc(this->pSendBuff, this->SendBuffSize);
		if (t == NULL) {
			//MyFree(this->pSendBuff);
			//this->pSendBuff = NULL;//失败会释放之前所有申请的内存
			MyDebug("Prealloc: Alloc Memory failed");
			return false;
		}
		this->pSendBuff = t;
	}
	return true;
}

void* MySocket::AllocBytes(size_t l)
{
	if (!this->Prealloc(l + sizeof(INT32)))
		return NULL;

	*(INT32*)((char*)this->pSendBuff + this->woff) = l;
	this->woff += sizeof(INT32);
	void* ret = (char*)this->pSendBuff + this->woff;
	this->woff += l;
	return ret;
}

bool MySocket::WriteString(const wchar_t* str)
{
	return this->WriteBytes(str, (MyStrLenW(str) + 1) * sizeof(wchar_t));
	//size_t l = (MyStrLenW(str) + 1) * sizeof(wchar_t);//包含终止符的总字节数
	//if (this->pSendBuff == NULL) {
	//	this->woff = 0;
	//	this->SendBuffSize = l;
	//	this->pSendBuff = MyAlloc(l);
	//}
	//else if (this->woff + l > this->SendBuffSize) {
	//	this->SendBuffSize += l;
	//	this->pSendBuff = MyReAlloc(this->pSendBuff, this->SendBuffSize);
	//}
	//if (this->pSendBuff == NULL) {
	//	MyDebug("WriteString: Alloc Memory failed");
	//	return false;
	//}
	//MyMemCpy((char*)this->pSendBuff + this->woff, str, l);
	//this->woff += l;
	//return true;
}

bool MySocket::WriteBytes(const void* buf, size_t l)
{
	if (!this->success)
		return false;
	void* ret = this->AllocBytes(l);
	if (ret == NULL) {
		this->success = false;
		return false;
	}
	MyMemCpy(ret, buf, l);
	return true;
	/*if (this->pSendBuff == NULL) {
		this->woff = sizeof(MySendPacketHead);
		this->SendBuffSize = sizeof(MySendPacketHead) + l + sizeof(INT32);
		this->pSendBuff = MyAlloc(this->SendBuffSize);
		if (this->pSendBuff == NULL) {
			MyDebug("WriteBytes: Alloc Memory failed");
			return false;
		}
	}
	else if (this->woff + l + sizeof(INT32) > this->SendBuffSize) {
		this->SendBuffSize = this->woff + l + sizeof(INT32);
		void* t = MyReAlloc(this->pSendBuff, this->SendBuffSize);
		if (t == NULL) {
			MyFree(this->pSendBuff);
			this->pSendBuff = NULL;
			MyDebug("WriteBytes: Alloc Memory failed");
			return false;
		}
		this->pSendBuff = t;
	}

	*(INT32*)((char*)this->pSendBuff + this->woff) = l;
	this->woff += sizeof(INT32);
	MyMemCpy((char*)this->pSendBuff + this->woff, buf, l);
	this->woff += l;
	return true;*/
}

bool MySocket::WriteUint32(UINT32 v)
{
	if (!this->success)
		return false;
	if (this->pSendBuff == NULL) {
		this->woff = sizeof(MySendPacketHead);
		this->SendBuffSize = sizeof(MySendPacketHead) + sizeof(UINT32);
		this->pSendBuff = MyAlloc(this->SendBuffSize);
		if (this->pSendBuff == NULL) {
			MyDebug("WriteUint32: Alloc Memory failed");
			this->success = false;
			return false;
		}
	}
	else if (this->woff + sizeof(UINT32) > this->SendBuffSize) {
		this->SendBuffSize = this->woff + sizeof(UINT32);
		void* t = MyReAlloc(this->pSendBuff, this->SendBuffSize);
		if (t == NULL) {
			MyFree(this->pSendBuff);
			this->pSendBuff = NULL;
			MyDebug("WriteUint32: Alloc Memory failed");
			this->success = false;
			return false;
		}
		this->pSendBuff = t;
	}


	*(UINT32*)((char*)this->pSendBuff + this->woff) = v;
	this->woff += sizeof(UINT32);
	return true;
}

void MySocket::Reset()
{
	this->success = true;
	if (this->pSendBuff != NULL)
		this->woff = sizeof(MySendPacketHead);
}

bool MySocket::Send(UINT32 Sign)
{
	if (!this->success)
		goto fail;
	if (this->sock != INVALID_SOCKET) {
		if (this->pSendBuff == NULL) {
			this->woff = sizeof(MySendPacketHead);
			this->SendBuffSize = sizeof(MySendPacketHead);
			this->pSendBuff = MyAlloc(this->SendBuffSize);
			if (this->pSendBuff == NULL) {
				goto fail;
			}
		}
		MySendPacketHead sph;
		sph.Sign = Sign;
		sph.Magic = PacketMagic;
		sph.PacketSize = this->woff - sizeof(MySendPacketHead);
		*(MySendPacketHead*)this->pSendBuff = sph;

		WaitForSingleObject(this->m, INFINITE);
		int c = 0; int sendLen = this->woff > sizeof(MySendPacketHead) ? this->woff : sizeof(UINT32);//当封包没有内容时只发送Sign,如心跳包
		do
		{
			int r = send(this->sock, (const char*)this->pSendBuff + c, sendLen - c, 0);
			if (r == SOCKET_ERROR) {
				MyDebug("Recv Error %d", WSAGetLastError());
				ReleaseMutex(this->m);
				goto fail;
			}
			c += r;
		} while (c < sendLen);
		ReleaseMutex(this->m);
		this->woff = sizeof(MySendPacketHead);
		return true;
	}
fail:
	this->Reset();
	return false;
}

bool MySocket::ReadString(wchar_t** str)
{
	void* buf; size_t l;
	if (!this->ReadBytes(&buf, &l))
		return false;
	if (*(wchar_t*)((char*)buf + l - sizeof(wchar_t)) != 0)//检查是否有终止符
		return false;
	*str = (wchar_t*)buf;
	return true;
	//if (this->pRecvBuff != NULL) {
	//	if (str == NULL) {
	//		MyDebug("Readstr failed: null pointer");
	//		return false;
	//	}
	//	if (this->roff + sizeof(INT32) >= this->RecvBodySize) {
	//		MyDebug("Readstr failed: not enough length 1");
	//		return false;
	//	}

	//	INT32 stringLen = *(INT32*)((char*)this->pRecvBuff + this->roff);
	//	this->roff += sizeof(INT32);

	//	if (stringLen <= 0) {
	//		MyDebug("Readstr failed: wrong length");
	//		return false;
	//	}
	//	if (this->roff + (UINT32)stringLen > this->RecvBodySize) {
	//		MyDebug("Readstr failed: not enough length 2");
	//		return false;
	//	}
	//	if (*(wchar_t*)((char*)this->pRecvBuff + this->roff + stringLen - sizeof(wchar_t))!=0) {
	//		MyDebug("Readstr failed: not end with \\0");
	//		return false;
	//	}

	//	*str = (wchar_t*)((char*)this->pRecvBuff + this->roff);
	//	this->roff += stringLen;
	//	return true;
	//}
	//return false;
}

bool MySocket::ReadBytes(void** buf, size_t* l)
{
	if (this->pRecvBuff != NULL) {
		if (buf == NULL || l == NULL) {
			MyDebug("ReadBytes failed: null pointer");
			return false;
		}
		if (this->roff + sizeof(INT32) >= this->RecvBodySize) {
			MyDebug("ReadBytes failed: not enough length 1");
			return false;
		}
		INT32 byteLen = *(INT32*)((char*)this->pRecvBuff + this->roff);
		this->roff += sizeof(INT32);
		
		if (byteLen<=0){
			MyDebug("ReadBytes failed: wrong length");
			return false;
		}
		if (this->roff + (UINT32)byteLen > this->RecvBodySize) {
			MyDebug("ReadBytes failed: not enough length 2");
			return false;
		}

		*buf = (char*)this->pRecvBuff + this->roff;
		*l = byteLen;

		this->roff += byteLen;
		return true;
	}
	return false;
}

bool MySocket::ReadUint32(UINT32* p)
{
	if (this->pRecvBuff != NULL) {
		if (p == NULL) {
			MyDebug("ReadUint32 failed: null pointer");
			return false;
		}
		if (this->roff + sizeof(UINT32) > this->RecvBodySize) {
			MyDebug("ReadUint32 failed: not enough length");
			return false;
		}

		*p = *(UINT32*)((char*)this->pRecvBuff + this->roff);
		this->roff += sizeof(UINT32);
		return true;
	}
	return false;
}

bool MySocket::Recv()
{
	if (this->sock != INVALID_SOCKET) {
		this->roff = 0;

		MyRecvPacketHead RecvHead;
		//int r,c=0;
		int r = recv(this->sock, (char*)&RecvHead, sizeof(MyRecvPacketHead), MSG_WAITALL);
		if (r == SOCKET_ERROR || r == 0) {//连接出错或连接正常关闭
			MyDebug("Recv RecvBody: Failed");
			return false;
		}
		//do
		//{
		//	r = recv(this->sock, (char*)&RecvHead + c, sizeof(MyRecvPacketHead) - c, NULL);
		//	if (r == SOCKET_ERROR) {//连接出错
		//		MyDebug("Recv RecvBody: Failed");
		//		return 0;
		//	}
		//	else if (r == 0) {//连接关闭
		//		MyDebug("Recv RecvBody: conn close");
		//		return 2;
		//	}
		//	c += r;
		//} while (c < sizeof(MyRecvPacketHead));

		if (RecvHead.Magic != PacketMagic) {
			MyDebug("Wrong PacketMagic");
			return false;
		}
		
		if (RecvHead.PacketSize <= 0) {
			MyDebug("Wrong PacketSize");
			return false;
		}

		if (this->pRecvBuff == NULL) {
			this->pRecvBuff = MyAlloc(RecvHead.PacketSize);
			if (this->pRecvBuff == NULL) {
				MyDebug("Recv: Alloc Memory failed");
				return false;
			}
		}else{
			size_t hs = MyMemSize(this->pRecvBuff);
			//RecvHead.PacketSize一定>0
			if (RecvHead.PacketSize > hs || hs > MaxPacketSize) {
				void* t = MyReAlloc(this->pRecvBuff, RecvHead.PacketSize);
				if (t == NULL) {
					MyFree(this->pRecvBuff);
					this->pRecvBuff = NULL;
					MyDebug("Recv: Alloc Memory failed");
					return false;
				}
				this->pRecvBuff = t;
			}
		}
		this->RecvBodySize = RecvHead.PacketSize;
		//c = 0;
		//do
		//{
		//	r = recv(this->sock, (char*)this->pRecvBuff + c, RecvHead.PacketSize - c, NULL);
		//	if (r == SOCKET_ERROR) {//连接出错
		//		MyDebug("Recv RecvBody Failed");
		//		return 0;
		//	}
		//	else if (r == 0) {//连接关闭
		//		MyDebug("Recv RecvBody: conn close");
		//		return 2;
		//	}
		//	c += r;
		//} while (c < RecvHead.PacketSize);
		r = recv(this->sock, (char*)this->pRecvBuff, RecvHead.PacketSize, MSG_WAITALL);
		if (r == SOCKET_ERROR || r == 0) {//连接出错或连接正常关闭
			MyDebug("Recv RecvBody Failed");
			return false;
		}
		return true;
	}
	return false;
}
