#pragma once

#include <Windows.h>
#include <WinSock2.h>

#define MaxPacketSize (50*1024*1024)//50MB
#define PacketMagic 0xcafefafa

typedef struct  MyRecvPacketHead
{
	UINT32 Magic;
	INT32 PacketSize;//Go中定义为uint32,一个数据包最大大小为2GB
};

typedef struct  MySendPacketHead
{
	UINT32 Sign;
	UINT32 Magic;
	INT32 PacketSize;
};

class MySocket {
private:
	SOCKET sock;

	void* pRecvBuff;
	size_t RecvBodySize;//封包body大小
	UINT32 roff;//已接收封包的当前读取位置

	void* pSendBuff;
	size_t SendBuffSize;//可写入的内存大小
	UINT32 woff;//待发送封包的当前写入位置

	HANDLE m;//类只保存由外部创建的互斥体,保证多线程发送、以及发送与释放资源不冲突

	bool success;
public:
	//MySocket()
	bool MySocketInit(HANDLE mu);//初始化最开始的类中变量的值,需要手动分配类空间
	void Free();//释放最开始的类(释放副本类的资源+共享的socket)
	MySocket* New();//复制出一个有相同socket和互斥锁的类,通过FreeCopy释放
	void FreeCopy();
	//~MySocket();
	bool Connect(const char* IP, u_short port);

	bool Prealloc(size_t l);//预先申请更多的用于存储发送内容的内存,提高连续写效率
	void* AllocBytes(size_t l);//用于存放用户自定义内容
	bool WriteString(const wchar_t* str);//失败会丢弃之前所有写入的内容,并且后续Write系列和Send均会失败,需要调用Reset重置
	bool WriteBytes(const void* buf, size_t l);//失败会丢弃之前所有写入的内容,并且后续Write系列和Send均会失败,需要调用Reset重置
	bool WriteUint32(UINT32 v);//失败会丢弃之前所有写入的内容,并且后续Write系列和Send均会失败,需要调用Reset重置
	//如果封包组织格式和服务器解析格式不同,会导致 后续发的心跳包被服务器当成当前封包内容来解析 或 封包未被解析的剩余内容被服务器当成下一个新封包来解析
	bool Send(UINT32 Sign);//一次性发送所有之前生成的数据,然后清除缓存,一定要检查返回值,失败自动重置
	void Reset();//重置发送封包的内容和发送状态

	bool ReadString(wchar_t** str);
	bool ReadBytes(void** buf, size_t* l);
	bool ReadUint32(UINT32* p);
	bool Recv();
 };