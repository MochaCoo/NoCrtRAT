#pragma once

#include <Windows.h>
#include <WinSock2.h>

#define MaxPacketSize (50*1024*1024)//50MB
#define PacketMagic 0xcafefafa

typedef struct  MyRecvPacketHead
{
	UINT32 Magic;
	INT32 PacketSize;//Go�ж���Ϊuint32,һ�����ݰ�����СΪ2GB
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
	size_t RecvBodySize;//���body��С
	UINT32 roff;//�ѽ��շ���ĵ�ǰ��ȡλ��

	void* pSendBuff;
	size_t SendBuffSize;//��д����ڴ��С
	UINT32 woff;//�����ͷ���ĵ�ǰд��λ��

	HANDLE m;//��ֻ�������ⲿ�����Ļ�����,��֤���̷߳��͡��Լ��������ͷ���Դ����ͻ

	bool success;
public:
	//MySocket()
	bool MySocketInit(HANDLE mu);//��ʼ���ʼ�����б�����ֵ,��Ҫ�ֶ�������ռ�
	void Free();//�ͷ��ʼ����(�ͷŸ��������Դ+�����socket)
	MySocket* New();//���Ƴ�һ������ͬsocket�ͻ���������,ͨ��FreeCopy�ͷ�
	void FreeCopy();
	//~MySocket();
	bool Connect(const char* IP, u_short port);

	bool Prealloc(size_t l);//Ԥ�������������ڴ洢�������ݵ��ڴ�,�������дЧ��
	void* AllocBytes(size_t l);//���ڴ���û��Զ�������
	bool WriteString(const wchar_t* str);//ʧ�ܻᶪ��֮ǰ����д�������,���Һ���Writeϵ�к�Send����ʧ��,��Ҫ����Reset����
	bool WriteBytes(const void* buf, size_t l);//ʧ�ܻᶪ��֮ǰ����д�������,���Һ���Writeϵ�к�Send����ʧ��,��Ҫ����Reset����
	bool WriteUint32(UINT32 v);//ʧ�ܻᶪ��֮ǰ����д�������,���Һ���Writeϵ�к�Send����ʧ��,��Ҫ����Reset����
	//��������֯��ʽ�ͷ�����������ʽ��ͬ,�ᵼ�� �������������������������ɵ�ǰ������������� �� ���δ��������ʣ�����ݱ�������������һ���·��������
	bool Send(UINT32 Sign);//һ���Է�������֮ǰ���ɵ�����,Ȼ���������,һ��Ҫ��鷵��ֵ,ʧ���Զ�����
	void Reset();//���÷��ͷ�������ݺͷ���״̬

	bool ReadString(wchar_t** str);
	bool ReadBytes(void** buf, size_t* l);
	bool ReadUint32(UINT32* p);
	bool Recv();
 };