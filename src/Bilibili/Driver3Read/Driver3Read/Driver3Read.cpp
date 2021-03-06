// Driver2Read.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <Windows.h>
#include <WinIoCtl.h>  // IoCtl 操作必须的头文件

#define IOCTL_TEST1 CTL_CODE(\
			FILE_DEVICE_UNKNOWN,\
			0x800,\
			METHOD_BUFFERED,\
			FILE_ANY_ACCESS)

int _tmain(int argc, TCHAR* argv[])
{
	HANDLE hDevice =
		CreateFileW(L"\\\\.\\MyRead",
			GENERIC_READ| GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		printf("Failed to obtain file handle to devices: Error %d\n", GetLastError());
		system("pause");
		return 1;
	}

	DWORD dwOutput = 0;
	UCHAR InputBuffer[10];
	UCHAR OutputBuffer[10];
	memset(OutputBuffer, 0, 10);
	memset(InputBuffer, 0xaa, 10);

	long pid = 0;
	printf("please enter the process pid:\n");
	scanf_s("%d", &pid);

	BOOL bRet = DeviceIoControl(hDevice, IOCTL_TEST1, &pid, 4, &OutputBuffer, 10, &dwOutput, NULL);
	if (bRet)
	{
		printf("Output Buffer %d bytes: ", dwOutput);
		for (int i = 0; i < (int)dwOutput; i++)
		{
			printf("%02X ", OutputBuffer[i]);
		}
		printf("\n");
	}
	CloseHandle(hDevice);
	system("pause");

    return 0;
}

