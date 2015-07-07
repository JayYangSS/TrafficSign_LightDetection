#include "socket_server_task.h"
/* 常量 */
#define DEFAULT_PORT "10004" // 端口
#define MAX_REQUEST 1024 // 接收数据的缓存大小
#define BUF_SIZE 4096 // 发送数据的缓存大小
/*发送标志位*/
bool gb_filled = false;
/*发送的数据*/
//float g_data = 0;	
CvMat *g_mat;
/*处理接收客户端连接线程*/
extern DWORD WINAPI Thread_AcceptHand(LPVOID lpParameter);
/*
处理任务的线程
发送图像和等数据
*/
DWORD WINAPI Thread_Task_Send(LPVOID lpParameter);

SOCKET ListenSocket = INVALID_SOCKET;// 监听socket
SOCKET ClientSocket = INVALID_SOCKET;// 连接socket
/*
初始化操作
*/
int SocketInit(void)
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		hints;
	int iResult;// 保存返回结果
	// 初始化Winsock，保证Ws2_32.dll已经加载
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 0;
	}
	// 地址
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 获取主机地址，保证网络协议可用等
	iResult = getaddrinfo(NULL, // 本机
		DEFAULT_PORT, // 端口
		&hints, // 使用的网络协议，连接类型等
		&result);// 结果
	if ( iResult != 0 )
	{
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 0;
	}

	// 创建socket，用于监听
	ListenSocket = socket(
		result->ai_family, // 网络协议，AF_INET，IPv4
		result->ai_socktype, // 类型，SOCK_STREAM
		result->ai_protocol);// 通信协议，TCP
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}
	// 绑定到端口
	iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}
	printf("bind\n");

	freeaddrinfo(result);// reuslt不再使用

	// 开始监听
	iResult = listen(ListenSocket, SOMAXCONN);
	printf("start listen......\n");
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}
	LPVOID lpParameter = NULL;
	/*创建线程接收连接*/
	CreateThread(
		NULL,
		0,
		Thread_AcceptHand, // 线程函数
		(LPVOID)lpParameter, // 将socket作为参数
		0,
		NULL);
}
/*处理接收客户端连接线程*/
DWORD WINAPI Thread_AcceptHand(LPVOID lpParameter)
{
	while(1)
	{
		// 接收客户端的连接，accept函数会等待，直到连接建立
		printf("ready to accept\n");
		ClientSocket = accept(ListenSocket, NULL, NULL);
		// accept函数返回，说明已经有客户端连接
		// 返回连接socket
		printf("accept a connetion\n");
		if (ClientSocket == INVALID_SOCKET)
		{
			printf("accept failed: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			break;// 等待连接错误，退出循环
		}
		// 为每一个连接创建一个数据发送的接收线程，
		// 使服务端又可以立即接收其他客户端的连接
		if(!CreateThread(
			NULL,
			0,
			Thread_Task_Send, // 线程函数
			(LPVOID)ClientSocket, // 将socket作为参数
			0,
			NULL))
		{
			printf("Create Thread error (%d)", GetLastError());
			break;
		}
	}
	// 循环退出，释放DLL。
	WSACleanup();
	return 0;
}
/*
处理任务的线程
发送图像和等数据
*/
DWORD WINAPI Thread_Task_Send(LPVOID lpParameter)
{
	DWORD dwTid = GetCurrentThreadId();
	// 获得参数sokcet
	SOCKET socket = (SOCKET)lpParameter;
	// 为接收数据分配空间
	int iResult;
	int bytesSent;// 用于保存send的返回值，实际发送的数据的大小
//	IplImage *p_src_img = cvLoadImage("lena.jpg", CV_LOAD_IMAGE_COLOR);
	//	IplImage *p_src_img = cvLoadImage("tiggo.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	char *txbuf = new char [2000000];
//	CvMat *p_mat = cvCreateMat(1,1, CV_32FC1);
	while(1)
	{
		int tx_num=0;
		while(!gb_filled)	//等待数据的装载
		{
			Sleep(10);		}
//		SocketPackIplImage((unsigned char *)txbuf, &tx_num, p_src_img);
//		bytesSent = send(socket, txbuf, tx_num, 0);
//		Sleep(10);
		
//		*(float *)CV_MAT_ELEM_PTR(*p_mat, 0, 0) = g_data;
		SocketPackArray((unsigned char *)txbuf, &tx_num, g_mat);

		bytesSent = send(socket, txbuf, tx_num, 0);
		Sleep(10);
		if( bytesSent == SOCKET_ERROR)
		{
			printf("\Thread_Task_Send\tsend error %d\n", 
				WSAGetLastError());
			closesocket(socket);
			return 1;
		}
		gb_filled = false;	//数据装载完毕
	}
//	cvReleaseMat(&p_mat);
	// 释放接收数据缓存，关闭socket
	delete [] txbuf;
	closesocket(socket);
	return 0;
}