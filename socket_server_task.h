#ifndef __SOCKET_SERVER_H
	#include "opencv2\opencv.hpp"
	#include "Socket_Protocol_Handle.h"
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>
	#pragma comment(lib, "Ws2_32.lib")	//socket

/*
	#pragma comment(lib, "opencv_core248d.lib")
	#pragma comment(lib, "opencv_highgui248d.lib")
	#pragma comment(lib, "opencv_imgproc248d.lib")
	#pragma comment(lib, "opencv_calib3d248d.lib")
	#pragma comment(lib, "opencv_contrib248d.lib")
	#pragma comment(lib, "opencv_features2d248d.lib")
	#pragma comment(lib, "opencv_flann248d.lib")
	#pragma comment(lib, "opencv_gpu248d.lib")
	#pragma comment(lib, "opencv_legacy248d.lib")
	#pragma comment(lib, "opencv_ml248d.lib")
	#pragma comment(lib, "opencv_objdetect248d.lib")
	#pragma comment(lib, "opencv_ts248d.lib")
	#pragma comment(lib, "opencv_video248d.lib")
	#pragma comment(lib, "opencv_core248.lib")
	#pragma comment(lib, "opencv_highgui248.lib")
	#pragma comment(lib, "opencv_imgproc248.lib")
	#pragma comment(lib, "opencv_calib3d248.lib")
	#pragma comment(lib, "opencv_contrib248.lib")
	#pragma comment(lib, "opencv_features2d248.lib")
	#pragma comment(lib, "opencv_flann248.lib")
	#pragma comment(lib, "opencv_gpu248.lib")
	#pragma comment(lib, "opencv_legacy248.lib")
	#pragma comment(lib, "opencv_ml248.lib")
	#pragma comment(lib, "opencv_objdetect248.lib")
	#pragma comment(lib, "opencv_ts248.lib")
	#pragma comment(lib, "opencv_video248.lib")
	*/
	/*发送标志位*/
	extern bool gb_filled;
	/*发送的数据*/
	extern CvMat *g_mat;
	//extern float g_data;	
	extern int SocketInit(void);
#endif