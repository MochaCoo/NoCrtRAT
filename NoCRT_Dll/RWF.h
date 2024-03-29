#pragma once
#include <windows.h>

void LongPath2W(const wchar_t* LongPath, wchar_t* Path);//ת��Ϊ���Դ����ĳ��ļ���·��,lstrlenW(Path)=_tcslen(LongPath)+lstrlenW(L"\\\\?\\")
BOOL SuperCreateDirectory(const wchar_t* Path);//�����༶Ŀ¼,������C:\a\b\c���ָ�ʽ,֧�ֳ��ļ���

class RWF{
	private:
		HANDLE fh;
	public:
		~RWF();

		BOOL openFile(
			LPCTSTR lpFileName,
			DWORD dwCreationDisposition = OPEN_EXISTING,
			DWORD dwDesiredAccess = GENERIC_READ | GENERIC_WRITE,
			DWORD dwShareMode = FILE_SHARE_READ,
			DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL
		);

		BOOL close();

		DWORD read(//�����㻺����
			LPVOID lpBuffer,
			SIZE_T nNumberOfBytesToRead = 0//��ͬ,ֻ��ʹ��HeapAlloc()������ڴ�ռ���Բ����������,���ҽ�Ĭ�ϻ�������СΪ��������Ŀռ�Ĵ�С
		);

		DWORD write(//�ڵ�ǰ�ļ�ָ�봦����ʽд������,�ļ�ָ���Զ�����д������ݵĳ���
			LPCVOID lpBuffer,
			SIZE_T nNumberOfBytesToWrite = 0
		);

		DWORD insert(//�ڵ�ǰ�ļ�ָ�봦��������,�ļ�ָ���Զ����ϲ�������ݵĳ���,����ʵ�ʲ��볤��,ʧ�ܷ���0
			LPCVOID lpBuffer,
			SIZE_T nNumberOfBytesToWrite = 0
		);

		DWORD del(LONG dwLength);//�ڵ�ǰָ�봦ɾ��ָ�����ȵ�����,������ָ��,����ʵ��ɾ������

		BOOL flushFileBuffers();//���ļ�����д�����

		DWORD setPointer(
			LONG lDistanceToMove,//��������õ�ֵ�����ļ���Χ��ָ�뽫�����õ��ļ�ĩβ(=FileSize)
			DWORD dwMoveMethod = FILE_BEGIN
		);

		DWORD getPointer();

		DWORD getFileSize();
};