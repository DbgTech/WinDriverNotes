// Driver2Read.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <Windows.h>

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

	UCHAR buffer[10];
	ULONG ulRead;
	BOOL bRet = ReadFile(hDevice, buffer, 10, &ulRead, NULL);
	if (bRet)
	{
		printf("Read %d bytes: ", ulRead);
		for (int i = 0; i < (int)ulRead; i++)
		{
			printf("%02X ", buffer[i]);
		}
		printf("\n");
	}
	CloseHandle(hDevice);
	system("pause");

    return 0;
}

