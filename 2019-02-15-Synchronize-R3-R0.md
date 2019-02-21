#同步#

###R3的同步###

在Ring3使用的同步机制都是内核提供的，主要包括事件，信号量，互斥体。这三个有各自的创建函数，获取它们则是使用相同的函数`WaitForSingleObject()`或`WaitForMultipleObjects()`，它们的使用示例如下代码段所示。

```
#include <windows.h>
#include <process.h>
#include <stddef.h>
#include <stdlib.h>
#include <conio.h>

UINT WINAPI	ThreadEvent(LPVOID para)
{
	printf("Enter ThreadEvent!\n");

	HANDLE *pEvent = (HANDLE*)para;
	if (NULL != pEvent)
		SetEvent(*pEvent);

	printf("Leave ThreadEvent!\n");
	return 0;
}

UINT WINAPI	ThreadSemaphere(LPVOID para)
{
	printf("Enter ThreadSemaphere!\n");
	HANDLE *pSemaphore = (HANDLE*)para;

	Sleep(2000);

	ReleaseSemaphore(*pSemaphore, 1, NULL);

	printf("Leave ThreadSemaphere!\n");
	return 0;
}

UINT WINAPI	ThreadMutex1(LPVOID para)
{
	printf("Enter ThreadMutex1\n");
	HANDLE *pMutex = (HANDLE*)para;
	if (pMutex)
	{
		WaitForSingleObject(*pMutex, INFINITE);

		WaitForSingleObject(*pMutex, INFINITE);

		printf("ThreadMutex1 Wait Two Time Mutex\n");

		Sleep(3000);
		printf("ThreadMutex1 Sleep comeback\n");

		ReleaseMutex(*pMutex);
	}
	printf("Leave ThreadMutex1\n");

	return 0L;
}

UINT WINAPI	ThreadMutex2(LPVOID para)
{
	printf("Enter ThreadMutex2\n");
	HANDLE *pMutex = (HANDLE*)para;
	if (pMutex)
	{
		printf("ThreadMutex2 Wait One Time Mutex Before\n");
		WaitForSingleObject(*pMutex, INFINITE);
		printf("ThreadMutex2 Wait One Time Mutex After\n");

		Sleep(3000);
		printf("ThreadMutex2 Sleep comeback\n");

		ReleaseMutex(*pMutex);
	}
	printf("Leave ThreadMutex2\n");

	return 0L;
}


int _tmain(int argc, _TCHAR* argv[])
{
	//----------------------------------------
	// Event
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	HANDLE hThreadEvent = (HANDLE)_beginthreadex(NULL, 0, ThreadEvent, &hEvent, 0, NULL);
	if (hThreadEvent)
		CloseHandle(hThreadEvent);

	WaitForSingleObject(hEvent, INFINITE);

	//----------------------------------------
	// Semaphore
	HANDLE hSemaphore = CreateSemaphore(NULL, 2, 2, NULL);
	if (hSemaphore)
	{
		WaitForSingleObject(hSemaphore, INFINITE);
		WaitForSingleObject(hSemaphore, INFINITE);
		HANDLE hThreadSemaphore = (HANDLE)_beginthreadex(NULL, 0, ThreadSemaphere, &hSemaphore, 0, NULL);
		if (hThreadSemaphore)
			CloseHandle(hThreadSemaphore);

		WaitForSingleObject(hSemaphore, INFINITE);
		printf("Main Thread get Semaphore Signal\n");
	}

	//-----------------------------------------
	// Mutex
	HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);
	if (hMutex)
	{
		HANDLE hThreadMutex1 = (HANDLE)_beginthreadex(NULL, 0, ThreadMutex1, &hMutex, 0, NULL);
		HANDLE hThreadMutex2 = (HANDLE)_beginthreadex(NULL, 0, ThreadMutex2, &hMutex, 0, NULL);

		HANDLE hThreads[2] = {};
		hThreads[0] = hThreadMutex1;
		hThreads[1] = hThreadMutex2;
		WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);
		CloseHandle(hThreads[0]);
		CloseHandle(hThreads[1]);
	}

	system("pause");

	return 0;
}
```

除了上述的这些方法外，在Ring3进行线程的“同步”或“互斥”还可以使用临界区（`CRITICAL_SECTION`），它只适用于进程内的线程之间的同步。`Interlocked*`系列函数则可以用于基本变量的互斥修改。

###R0的同步###

**自旋锁**

它是一种同步处理机制，能够保证某个资源只能被一个线程所拥有。自旋锁如其名字含义，如果有程序申请获取的自旋锁被锁住了，那么这个线程就进入了自旋状态。所谓自旋状态就是不停询问是否可以获取自旋锁。

它与等待事件不同，自旋状态不会让当前线程进入休眠，而是一直处于自旋，这不太适合与长时间占用锁的逻辑。


```
KSPIN_LOCK MySpinLock;  // 定义自旋锁对象，一般要定义为全局的。

KIRQL oldirql;
KeAcquireSpinLock(&MySpinLock, &oldirql);

KeReleaseSpinLock(&MySpinLock, &oldirql);
```

**内核同步**

在内核中也有两个函数负责等待内核同步对象，与Ring3层的两个函数类似，它们是`KeWaitForSingleObject()`和`KeWaitForMultipleObject()`。

内核中创建线程的函数为`PsCreateSystemThread()`，其参数`ProcessHandle`表示线程的归属，如果这个参数为NULL，那么创建的线程是系统线程，从属于`Idle`进程；如果值为进程句柄，则创建的线程归属于该进程。






By Andy@2019-02-19 18:42:32