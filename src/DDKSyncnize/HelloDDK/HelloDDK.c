
#include "HelloDDK.h"

WCHAR* s_lpDeviceName = L"\\Device\\MyDDKDevice";
WCHAR* s_lpSymbolicName = L"\\??\\HelloDDK";

#define ID_IOCTL_TEST1 CTL_CODE(\
						FILE_DEVICE_UNKNOWN,\
						0x800,\
						METHOD_BUFFERED,\
						FILE_ANY_ACCESS)

#define ID_IOCTL_TRANSMIT_EVENT CTL_CODE(\
						FILE_DEVICE_UNKNOWN,\
						0x801,\
						METHOD_BUFFERED,\
						FILE_ANY_ACCESS)

#pragma PAGECODE
VOID SystemThread(IN PVOID pContext)
{
	UNREFERENCED_PARAMETER(pContext);
	KdPrint(("Enter SystemThread\n"));

	PEPROCESS pEProcess = IoGetCurrentProcess();
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);
	KdPrint(("This thread run in %s process\n", ProcessName));

	KdPrint(("Leave SystemThread\n"));

	PsTerminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGECODE
VOID ProcessThread(IN PVOID pContext)
{
	UNREFERENCED_PARAMETER(pContext);
	KdPrint(("Enter ProcessThread\n"));

	PEPROCESS pEProcess = IoGetCurrentProcess();
	PTSTR ProcessName = (PTSTR)((ULONG)pEProcess + 0x174);
	KdPrint(("This thread run in %s process\n", ProcessName));

	KdPrint(("Leave ProcessThread\n"));

	PsTerminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGECODE
VOID CreateThread_Test()
{
	HANDLE hSystemThread, hProcThread;

	NTSTATUS status = PsCreateSystemThread(&hSystemThread, 0, NULL, NULL, NULL, SystemThread, NULL);
	status = PsCreateSystemThread(&hProcThread, 0, NULL, NtCurrentProcess(), NULL, ProcessThread, NULL);
}

#pragma INITCODE 
NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;	
	KdPrint(("Enter DriverEntry %wZ\n", pRegistryPath));

	pDriverObject->DriverUnload = HelloDDKUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = HelloDDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HelloDDKIoCtlRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = HelloDDKDispatchRoutine;	

	status = CreateDevice(pDriverObject);

	KdPrint(("Leave DriverEntry\n"));
	return status;
}

#pragma INITCODE
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	// �����豸��
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, s_lpDeviceName);
	status = IoCreateDevice(pDriverObject,			// �����豸
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0, TRUE,
		&pDevObj);
	if (!NT_SUCCESS(status))
		return status;

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	// ��������
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, s_lpSymbolicName);
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}

	return STATUS_SUCCESS;
}

#pragma PAGECODE
VOID HelloDDKUnload(IN PDRIVER_OBJECT pDriverObject)
{
	PDEVICE_OBJECT pNextObj;
	KdPrint(("Enter DriverUnload\n"));
	pNextObj = pDriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		// ɾ������
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}
	KdPrint(("Leave DriverUnload\n"));
}

#pragma PAGECODE
NTSTATUS HelloDDKDispatchRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKDispatchRoutine DevObj: %p\n", pDevObj));
	NTSTATUS status = STATUS_SUCCESS;

	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKDispatchRoutine\n"));
	return status;
}

#pragma PAGECODE
VOID EventTestThread(IN PVOID pContext)
{
	PKEVENT pEvent = (PKEVENT)pContext;
	KdPrint(("Enter EventTestThread\n"));
	if (pEvent)
	{
		KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
	}
	KdPrint(("Leave EventTestThread\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGECODE
VOID EventTest()
{
	HANDLE hMyThread;
	KEVENT hEvent;

	KeInitializeEvent(&hEvent, NotificationEvent, FALSE);
	NTSTATUS status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, EventTestThread, &hEvent);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Create Process Thread Error!\n"));
	}

	KeWaitForSingleObject(&hEvent, Executive, KernelMode, FALSE, NULL);
	KdPrint(("Event has Single\n"));
}

#pragma PAGECODE
VOID SetUserEvent(HANDLE hEvent)
{
	if (hEvent == NULL)
		return;
	
	PKEVENT pEvent = NULL;
	NTSTATUS Status = ObReferenceObjectByHandle(hEvent, EVENT_MODIFY_STATE, *ExEventObjectType, KernelMode, (PVOID*)&pEvent, NULL);
	if (NT_SUCCESS(Status) && pEvent != NULL)
	{
		KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);
		ObDereferenceObject(pEvent);
	}
}

#pragma PAGECODE
VOID SemaphoreThread(IN PVOID pContext)
{
	PKSEMAPHORE pkSemaphore = (PKSEMAPHORE)pContext;
	KdPrint(("Enter Semaphore Thread\n"));
	if (pkSemaphore)
	{
		KeReleaseSemaphore(pkSemaphore, IO_NO_INCREMENT, 1, FALSE);
	}
	KdPrint(("Leave Semaphore Thread\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
}

#pragma PAGECODE
VOID SemaphoreTest()
{
	HANDLE hMyThread;
	KSEMAPHORE kSemaphore;

	KeInitializeSemaphore(&kSemaphore, 2, 2);
	LONG count = KeReadStateSemaphore(&kSemaphore);
	KdPrint(("The Semaphore count is %d\n", count));
	KeWaitForSingleObject(&kSemaphore, Executive, KernelMode, FALSE, NULL);
	count = KeReadStateSemaphore(&kSemaphore);
	KdPrint(("The Semaphore count is %d\n", count));
	KeWaitForSingleObject(&kSemaphore, Executive, KernelMode, FALSE, NULL);

	NTSTATUS status = PsCreateSystemThread(&hMyThread, 0, NULL, NtCurrentProcess(), NULL, SemaphoreThread, &kSemaphore);
	if (NT_SUCCESS(status))
	{
		ZwClose(hMyThread);
	}
	KeWaitForSingleObject(&kSemaphore, Executive, KernelMode, FALSE, NULL);
	KdPrint(("After KeWaitForSingleObject\n"));

	return ;
}

VOID __stdcall MutexThread1(PVOID lpParam)
{
	KdPrint(("Enter MutexThread1\n"));
	PKMUTEX pKMutex = (PKMUTEX)lpParam;
	if (pKMutex)
	{
		KeWaitForSingleObject(pKMutex, Executive, KernelMode, FALSE, NULL);
		KdPrint(("MutexThread1 After Wait For Mutext!\n"));

		KeStallExecutionProcessor(50);
		KdPrint(("MutexThread1 After stall Execute!\n"));

		KeReleaseMutex(pKMutex, FALSE);
	}
	
	KdPrint(("Leave MutexThread1\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);	
}

VOID __stdcall MutexThread2(PVOID lpParam)
{
	KdPrint(("Enter MutexThread2\n"));
	PKMUTEX pKMutex = (PKMUTEX)lpParam;
	if (pKMutex)
	{
		KeWaitForSingleObject(pKMutex, Executive, KernelMode, FALSE, NULL);
		KdPrint(("MutexThread2 After Wait For Mutext!\n"));

		KeStallExecutionProcessor(50);
		KdPrint(("MutexThread2 After stall Execute!\n"));

		KeReleaseMutex(pKMutex, FALSE);
	}

	KdPrint(("Leave MutexThread2\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);	
}

#pragma PAGECODE
VOID MutexTest()
{
	HANDLE hMyThread1, hMyThread2;
	KMUTEX kMutex;
	KdPrint(("Enter MutexTest Func\n"));
	KeInitializeMutex(&kMutex, 0);

	NTSTATUS status = PsCreateSystemThread(&hMyThread1, 0, NULL, NtCurrentProcess(), NULL, MutexThread1, &kMutex);
	status = PsCreateSystemThread(&hMyThread2, 0, NULL, NtCurrentProcess(), NULL, MutexThread2, &kMutex);
	PVOID Pointer_Array[2];
	if (hMyThread1)
	{
		ObReferenceObjectByHandle(hMyThread1, 0, NULL, KernelMode, &Pointer_Array[0], NULL);
		ZwClose(hMyThread1);
	}
	if (hMyThread2)
	{
		ObReferenceObjectByHandle(hMyThread2, 0, NULL, KernelMode, &Pointer_Array[1], NULL);
		ZwClose(hMyThread2);
	}

	KeWaitForMultipleObjects(2, Pointer_Array, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);
	ObDereferenceObject(Pointer_Array[0]);
	ObDereferenceObject(Pointer_Array[1]);
	
	KdPrint(("Leave MutexTest Func\n"));
}

#pragma PAGECODE
NTSTATUS HelloDDKIoCtlRoutine(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("Enter HelloDDKIoCtlRoutine DevObj: %p\n", pDevObj));
	NTSTATUS status = STATUS_SUCCESS;

	PIO_STACK_LOCATION pIoStack = IoGetCurrentIrpStackLocation(pIrp);
	//ULONG cbInBuffer = pIoStack->Parameters.DeviceIoControl.InputBufferLength;
	//ULONG cbOutBuffer = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;

	ULONG ctlCode = pIoStack->Parameters.DeviceIoControl.IoControlCode;
	switch (ctlCode)
	{
	case ID_IOCTL_TEST1:
	{
		KdPrint(("Create Thread: \n"));
		CreateThread_Test();

		KdPrint(("Event Test: \n"));
		EventTest();

		KdPrint(("Semaphore Test: \n"));
		SemaphoreTest();

		KdPrint(("Mutex Test:\n"));
		MutexTest();
	}
	case ID_IOCTL_TRANSMIT_EVENT:
	{
		KdPrint(("Start Thread Set User Event:"));
		HANDLE hEvent = (HANDLE)*(LONG_PTR*)pIrp->AssociatedIrp.SystemBuffer;
		SetUserEvent(hEvent);
	}
	default:
		break;
	}

	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	KdPrint(("Leave HelloDDKIoCtlRoutine\n"));
	return status;
}

