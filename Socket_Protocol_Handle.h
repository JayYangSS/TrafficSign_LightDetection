/**************************************************************************
文件名	：Socket_Protocol_Handle.h
作者	：dly
版本	：2.0
修改时间：2014-08-24
功能	：根据《网络数据传输协议-2.0》实现按协议封装数据包,解析数据包功能
总的传输格式：包头数据类型码 + 子类型码(1) + 时间戳(4) + 报文长度(4) + 传输数据 + 包尾数据类型码（同包头）
***********************************************************************/
#ifndef __SOCKET_PROTOCOL_HANDLE_H
#define __SOCKET_PROTOCOL_HANDLE_H
#include "opencv2\opencv.hpp"
/*类型*/
#define INDEX_NULL		0	//无数据
#define INDEX_STRING	1	//字符串
#define INDEX_ARRAY		2	//数组
#define INDEX_IMAGE		3	//图像
#define INDEX_USER		4	//用户自定义
/*子类型*/
#define SUB_INDEX_1		1	//
/*错误类型*/
#define ERR_FAULT		-1	//包内协议数据出错
#define ERR_OK			0	//解析完成
#define ERR_STICKING	1	//package sticking
#define ERR_BROKEN		2	//package broken


/*保存协议解析结果结构体*/
typedef 
struct _DECODING_RESULT
{
	unsigned char index;	//数据类型
	unsigned char sub_index;//子数据类型
	unsigned int timestamp;	//时间错
	union	//数据指针
	{
		struct
		{
			char *p_string;		//字符串
			int length;
		};
		CvMat *p_mat;		//矩阵mat
		IplImage *p_img;	//图像
	};
	int total_byte_num;	//有效数据总的字节数
}
DECODING_RESULT,SEND_OBJ;

extern DECODING_RESULT g_socket_decoding_result;
extern char g_err_info[128];
extern int SocketDecoder(DECODING_RESULT *p_decoding_result, unsigned char * p_rxbuf, int rx_num, char *err_info);
extern int SocketPackager(unsigned char *p_txbuf, int *tx_num, SEND_OBJ *p_send_obj);
extern int SocketPackString(unsigned char *const p_txbuf, int *const p_tx_num, char *const p_string, int length, unsigned char sub_index=0, unsigned int timestamp=0);
extern int SocketPackIplImage(unsigned char *const p_txbuf, int *const p_tx_num, IplImage *const p_img, unsigned char sub_index=0, unsigned int timestamp=0);
extern int SocketPackArray(unsigned char *const p_txbuf, int *const p_tx_num, CvMat * const p_cv_mat, unsigned char sub_index=0, unsigned int timestamp=0);
extern int ReleaseData_DecodingResultObj(DECODING_RESULT *p_obj);
#endif