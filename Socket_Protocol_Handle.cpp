/**************************************************************************
文件名	：Socket_Protocol_Handle.cpp
作者	：dly
版本	：2.0
修改时间：2014-08-24//新增解决断包问题
功能	：根据《网络数据传输协议-2.0》实现按协议封装数据包,解析数据包功能
总的传输格式：包头数据类型码 + 子类型码(1) + 时间戳(4) + 报文长度(4) + 传输数据 + 包尾数据类型码（同包头）
***********************************************************************/
#include "Socket_Protocol_Handle.h"	
char g_err_info[128];	//用于输出错误信息
DECODING_RESULT g_socket_decoding_result;
SEND_OBJ g_package_obj;
/**************************解析数据包***************************/

/*
功能	：解析接收数据
输入	：&1：解析结果指针	&2：输入缓存指针  &3：缓存长度
返回值	：解析完成标志：0表示解析完成，1表示保内还有数据待解析,-1解析错误
备注	：函数会为数据分配空间，用完后需释放空间
*/
int SocketDecoder(DECODING_RESULT *const p_decoding_result, unsigned char *const p_rxbuf, int rx_num, char *const err_info)
{
	if(rx_num < 20)
	{
		strcpy(err_info, "rxbuf is two short!\n");
		p_decoding_result->index = INDEX_NULL;
		return ERR_FAULT;
	}
	if(strcmp((char *)p_rxbuf, "STRING")==0)//检查包头是否为字符串
	{
		//读取数据长度
		unsigned int *p_len = (unsigned int *)(p_rxbuf+sizeof("STRING")+1+4);
		p_decoding_result->total_byte_num = sizeof("STRING")*2+9+*p_len;
		if(rx_num < p_decoding_result->total_byte_num)	//校验数据长度
		{
			//接收数据长度错误处理
			strcpy(err_info, "STRING:Broken!");
			p_decoding_result->index = INDEX_NULL;
			return ERR_BROKEN;
		}
		else if(strcmp((char *)(p_rxbuf+sizeof("STRING")+9+*p_len), "STRING")!=0)	//检查包尾
		{
			//包尾错误处理
			strcpy(err_info, "error in STRING message end!\n");
			p_decoding_result->index = INDEX_NULL;
			return ERR_FAULT;
		}
		else if(*(p_rxbuf+sizeof("STRING")+9+*p_len-1)!='\0')	//检查字符串'\0'
		{
			strcpy(err_info, "error in string message data\n");
			p_decoding_result->index = INDEX_NULL;
			return ERR_FAULT;
		}
		else	//正确
		{
			p_decoding_result->index = INDEX_STRING;
			p_decoding_result->sub_index = *(p_rxbuf+sizeof("STRING"));
			p_decoding_result->timestamp = *(unsigned int *)(p_rxbuf+sizeof("STRING")+1);
			p_decoding_result->length = *(unsigned int *)(p_rxbuf+sizeof("STRING")+5);
			p_decoding_result->p_string = new char [p_decoding_result->length];	//动态分配空间
			memcpy(p_decoding_result->p_string, (p_rxbuf+sizeof("STRING")+9), p_decoding_result->length);
			strcpy(err_info, "string ok!\n");
			if(rx_num > p_decoding_result->total_byte_num)
				return ERR_STICKING;
			else
				return ERR_OK;
		}
	}
	else if(strcmp((char *)p_rxbuf ,"ARRAY")==0)//数组
	{
		//读取数据长度
		unsigned int *p_len = (unsigned int *)(p_rxbuf+sizeof("ARRAY")+5);
		p_decoding_result->total_byte_num = sizeof("ARRAY")*2+9+*p_len;
		if(rx_num < p_decoding_result->total_byte_num)	//接收长度小于指定长度报错
		{
			//数据长度错误处理
			strcpy(err_info,"ARRAY:Broken!");
			p_decoding_result->index = INDEX_NULL;
			return ERR_BROKEN;
		}
		else if(strcmp((char *)(p_rxbuf+sizeof("ARRAY")+9+*p_len), "ARRAY")!=0)	//包尾
		{
			//包尾错误处理
			strcpy(err_info,"error in ARRAY end\n");
			p_decoding_result->index = INDEX_NULL;
			return ERR_FAULT;
		}
		else
		{
			unsigned char *p_data = (unsigned char *)(p_rxbuf+sizeof("IMAGE")+9);
			unsigned int rows = *(unsigned int *)p_data;
			unsigned int  cols = *(unsigned int *)(p_data+4);
			int type = *(int *)(p_data+8);
			CvMat *& p_mat = p_decoding_result->p_mat;
			p_mat = cvCreateMat(rows, cols, type);
			if((unsigned int)(*p_len - 12) != p_mat->step*p_mat->rows)
			{
				cvReleaseMat(&p_mat);
				strcpy(err_info,"error in ARRAY property\n");		
				p_decoding_result->index = INDEX_NULL;
				return ERR_FAULT;
			}
			else
			{
				memcpy(p_mat->data.ptr, (p_data+12), p_mat->step*p_mat->rows);	//注必须用widthStep
				p_decoding_result->index = INDEX_ARRAY;
				p_decoding_result->sub_index = *(p_rxbuf+sizeof("ARRAY"));
				p_decoding_result->timestamp = *(unsigned int *)(p_rxbuf+sizeof("ARRAY")+1);
				strcpy(err_info,"ARRAY ok!\n");
				if(rx_num > p_decoding_result->total_byte_num)
					return ERR_STICKING;
				else
					return ERR_OK;
			}

		}
	}
	else if(strcmp((char *)p_rxbuf, "IMAGE")==0)//图像
	{
		//读取数据长度
		unsigned int *p_len = (unsigned int *)(p_rxbuf+sizeof("IMAGE")+5);
		p_decoding_result->total_byte_num = sizeof("IMAGE")*2+9+*p_len;
		if(rx_num < p_decoding_result->total_byte_num)	//接收长度小于指定长度报错
		{
			//数据长度错误处理
			p_decoding_result->index = INDEX_NULL;
			strcpy(err_info,"image:Broken!");
			return ERR_BROKEN;
		}
		else if(strcmp((char *)(p_rxbuf+sizeof("IMAGE")+9+*p_len), "IMAGE")!=0)	//包尾
		{
			//包尾错误处理
			strcpy(err_info,"error in image end\n");
			p_decoding_result->index = INDEX_NULL;
			return ERR_FAULT;
		}
		else
		{
			unsigned char *p_data = (unsigned char *)(p_rxbuf+sizeof("IMAGE")+9);
			unsigned int width = *(unsigned int *)p_data;
			unsigned int  height = *(unsigned int *)(p_data+4);
			unsigned char channels = *(unsigned char *)(p_data+8);
			unsigned char depth = *(unsigned char *)(p_data+9);
			IplImage *&p_img = p_decoding_result->p_img;
			p_img = cvCreateImage(cvSize(width, height), depth, channels); 
			if((unsigned int)(*p_len - 10) != p_img->widthStep*p_img->height)	//简单判断IplImage中的内容
			{
				cvReleaseImage(&p_img);
				strcpy(err_info,"error in image property\n");
				p_decoding_result->index = INDEX_NULL;			
				return ERR_FAULT;
			}
			else
			{
				memcpy(p_img->imageData, (p_data+10), p_img->widthStep*p_img->height);	//注必须用widthStep
				p_decoding_result->index = INDEX_IMAGE;
				p_decoding_result->sub_index = *(p_rxbuf+sizeof("IMAGE"));
				p_decoding_result->timestamp = *(unsigned int *)(p_rxbuf+sizeof("IMAGE")+1);
				strcpy(err_info,"image ok!\n");
				if(rx_num > p_decoding_result->total_byte_num)
					return ERR_STICKING;
				else
					return ERR_OK;
			}
		}
	}	
	else if(strcmp((char *)p_rxbuf, "USER")==0)//用户
	{
		return ERR_OK;
	}
	else//其他数据
	{
		strcpy(err_info, "error message!\n");
		p_decoding_result->index = INDEX_NULL;
		return ERR_FAULT;
	}
}

/*
释放DECODING_RESULT结构体指针内的数据指针,并不对结构体头做处理
*/
int ReleaseData_DecodingResultObj(DECODING_RESULT *p_obj)
{
	if(p_obj->index == INDEX_STRING)
	{
		if(p_obj->p_string!=NULL)
		{
			delete [] p_obj->p_string;	
		}
	}
	else if(p_obj->index == INDEX_IMAGE)
	{
		if(p_obj->p_img!=NULL)
		{
			cvReleaseImage(&p_obj->p_img);
		}
	}
	else if(p_obj->index == INDEX_ARRAY)
	{
		if(p_obj->p_mat!=NULL)
		{
			cvReleaseMat(&p_obj->p_mat);
		}
	}
	return ERR_OK;
}

/*
功能	：封装指定类型的数据
输入	：&1：发送缓存指针	&2：长度指针	&3：发送数据的结构体指针
返回值	：封装成功标志
备注	：发送缓存大小要足够
*/
int SocketPackager(unsigned char *p_txbuf, int *p_tx_num, SEND_OBJ *p_pack_obj)
{
	if(p_pack_obj->index == INDEX_STRING)	//封装字符串
	{
		int i = 0;
		strcpy((char *)p_txbuf, "STRING");	//包头
		i += 7;
		p_txbuf[i] = p_pack_obj->sub_index;	//子类型
		i += 1;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->timestamp;	//时间戳
		i += 4;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->length;		//数据长度
		i += 4;
		memcpy(p_txbuf+i, p_pack_obj->p_string, p_pack_obj->length);	//copy数据
		i += p_pack_obj->length;
		strcpy((char *)(p_txbuf+i), "STRING");	//包尾
		i += 7;
		*p_tx_num = i;	//发送缓存长度
		return ERR_OK;
	}
	else if(p_pack_obj->index == INDEX_ARRAY)
	{
		int i = 0;
		strcpy((char *)p_txbuf, "ARRAY");	//包头
		i += 6;
		p_txbuf[i] = p_pack_obj->sub_index;	//子类型
		i += 1;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->timestamp;	//时间戳
		i += 4;
		int mat_bytes_num = p_pack_obj->p_mat->step * p_pack_obj->p_mat->rows;	//mat所占的字节数
		//		printf("%d,%d,%d",p_pack_obj->p_mat->step, p_pack_obj->p_mat->rows, p_pack_obj->p_mat->cols);
		*(unsigned int *)(p_txbuf+i) = mat_bytes_num + 12;		//数据长度，得用step才行
		i += 4;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->p_mat->rows;	//行数
		i += 4;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->p_mat->cols;	//列数
		i += 4;
		*(int *)(p_txbuf+i) = p_pack_obj->p_mat->type;	//矩阵类型
		i += 4;
		memcpy(p_txbuf+i, p_pack_obj->p_mat->data.ptr, mat_bytes_num);	//copy数据
		i += mat_bytes_num;
		strcpy((char *)(p_txbuf+i), "ARRAY");	//包尾
		i += 6;
		*p_tx_num = i;	//发送缓存长度
		return ERR_OK;

	}
	else if(p_pack_obj->index == INDEX_IMAGE)
	{
		int i=0, j=0;
		unsigned int *len;
		IplImage * &p_src_img = p_pack_obj->p_img;	
		int image_bytes_num = p_src_img->widthStep*p_src_img->height;	//IplImage图像数据占的字节数
		//IMAGE
		strcpy((char *)p_txbuf, "IMAGE");	//包头
		i += 6;
		p_txbuf[i] = p_pack_obj->sub_index;	//子类型
		i += 1;
		*(unsigned int *)(p_txbuf+i) = p_pack_obj->timestamp;	//时间戳
		i += 4;
		len = (unsigned int*)(p_txbuf + i);
		*len = image_bytes_num+4+4+1+1;
		i = i+4;	//长度
		*(unsigned int*)(p_txbuf + i) = p_src_img->width;
		i = i+4;
		*(unsigned int*)(p_txbuf + i) = p_src_img->height;
		i = i+4;
		*(unsigned int*)(p_txbuf + i) = p_src_img->nChannels;
		i = i+1;
		*(unsigned int*)(p_txbuf + i) = p_src_img->depth;
		i = i+1;
		memcpy(&p_txbuf[i], p_src_img->imageData, image_bytes_num);
		i = i+ image_bytes_num;
		strcpy((char *)(p_txbuf+i), "IMAGE");	//包尾
		i += 6;
		*p_tx_num = i;	//发送缓存长度
		return ERR_OK;
	}
	return ERR_FAULT;
}
/*
功能	：封装字符串到发送缓存
输入	：&1：发送缓存指针	&2：长度指针	&3:发送结构体，&4：发送的字符串 &5：字符串长度 &6：子类型 &7：时间戳
返回值	：封装成功标志
备注	：发送缓存大小要足够
*/
int SocketPackString(unsigned char *const p_txbuf, int *const p_tx_num, char *const p_string, int length, unsigned char sub_index, unsigned int timestamp)
{
	SEND_OBJ package_obj;
	package_obj.index = INDEX_STRING;
	package_obj.sub_index = sub_index;
	package_obj.timestamp = timestamp;
	package_obj.p_string = p_string;
	package_obj.length = length;
	SocketPackager(p_txbuf, p_tx_num, &package_obj);
	return ERR_OK;
}
/*
功能	：封装IplImage到发送缓存
输入	：&1：发送缓存指针	&2：长度指针 &3：发送结构体 &4：发送的图像
返回值	：封装成功标志
备注	：发送缓存大小要足够
*/
int SocketPackIplImage(unsigned char *const p_txbuf, int *const p_tx_num, IplImage *const p_img, unsigned char sub_index, unsigned int timestamp)
{
	SEND_OBJ package_obj;
	package_obj.index = INDEX_IMAGE;
	package_obj.sub_index = sub_index;
	package_obj.timestamp = timestamp;
	package_obj.p_img = p_img;
	SocketPackager(p_txbuf, p_tx_num, &package_obj);
	return ERR_OK;
}
/*
功能	：封装CvMat到发送缓存
输入	：&1：发送缓存指针	&2：长度指针	&3:发送结构体，&4：CvMat指针 &5：子类型 &6：时间戳
返回值	：封装成功标志
备注	：发送缓存大小要足够
*/
int SocketPackArray(unsigned char *const p_txbuf, int *const p_tx_num, CvMat * const p_cv_mat, unsigned char sub_index, unsigned int timestamp)
{
	SEND_OBJ package_obj;
	package_obj.index = INDEX_ARRAY;
	package_obj.sub_index = sub_index;
	package_obj.timestamp = timestamp;
	package_obj.p_mat = p_cv_mat;
	SocketPackager(p_txbuf, p_tx_num, &package_obj);
	return ERR_OK;	
}

/*************************************************************************************************************************************************************
////****使用范例
*************************************************************************************************************************************************************/
/*
/////////////////发送报文//////////////////
while(1)
{
	Sleep(4);
	int tx_num=0;
	SocketPackString((unsigned char *)txbuf, &tx_num, "i am dly!", sizeof("i am dly!"));
	bytesSent = send(socket, txbuf, tx_num, 0);
	if( bytesSent == SOCKET_ERROR)
	{
		printf("\tCommunicationThread\tsend error %d\n", 
			WSAGetLastError());
		closesocket(socket);
		return 1;
	}
	SocketPackIplImage((unsigned char *)txbuf, &tx_num, p_src_img);
	bytesSent = send(socket, txbuf, tx_num, 0);
	//		Sleep(5);
	if( bytesSent == SOCKET_ERROR)
	{
		printf("\tCommunicationThread\tsend error %d\n", 
			WSAGetLastError());
		closesocket(socket);
		return 1;
	}
	CvMat *p_mat = cvCreateMat(5,3, CV_32FC1);
	static int x=0;
	for(int j=0; j<p_mat->rows; j++)
	{
		for(int i=0; i<p_mat->cols; i++)
		{
			*(float *)CV_MAT_ELEM_PTR(*p_mat, j, i) = x++;
		}
	}
	SocketPackArray((unsigned char *)txbuf, &tx_num, p_mat);
	cvReleaseMat(&p_mat);
	bytesSent = send(socket, txbuf, tx_num, 0);
	//		Sleep(1);
	if( bytesSent == SOCKET_ERROR)
	{
		printf("\tCommunicationThread\tsend error %d\n", 
			WSAGetLastError());
		closesocket(socket);
		return 1;
	}
}
/////////////////////////////接收报文///////////////////////////////
while(1)
{
	// 接收数据
	int bytesRecv = recv(socket, // socket
		rxbuf, // 接收缓存
		RECV_BUF_SIZE, // 缓存大小
		0);// 标志
	printf("recv_byte=%d\n",bytesRecv);
	if (bytesRecv == 0)// 接收数据失败，连接已经关闭
	{
		printf("Connection closing...\n");
		closesocket(socket);
		return 1;
	}
	else if (bytesRecv == SOCKET_ERROR)// 接收数据失败，socket错误
	{
		printf("recv failed: %d\n", WSAGetLastError());
		closesocket(socket);
		return 1;
	}
	else if(bytesRecv>0)
	{
		int result = ERR_STICKING;
		int rx_num = bytesRecv;
		char *p_buf = rxbuf;
		g_socket_decoding_result.total_byte_num = 0;	
		while(result == ERR_STICKING || result == ERR_BROKEN)
		{
			if(result == ERR_STICKING)	
			{
				result = SocketDecoder(&g_socket_decoding_result, (unsigned char*)p_buf, rx_num, g_err_info);
				if(result == ERR_FAULT)printf(g_err_info);
				if(result != ERR_OK && result != ERR_STICKING)	//如果没有接收都有效的数据包跳出本次循环
					continue;
				p_buf += g_socket_decoding_result.total_byte_num;
				rx_num -= g_socket_decoding_result.total_byte_num;
				if (g_socket_decoding_result.index == INDEX_STRING)
				{
#ifdef DEBUG_DISPLAY_STRING
					printf(g_socket_decoding_result.p_string);
#endif
				}
				else if(g_socket_decoding_result.index == INDEX_ARRAY)
				{
					CvMat *&p_mat = g_socket_decoding_result.p_mat;
#ifdef DEBUG_DISPLAY_ARRAY
					printf("sub_index=%d,timestamp=%d\n", g_socket_decoding_result.sub_index, g_socket_decoding_result.timestamp);
#endif
					for(int j=0; j<p_mat->rows; j++)
					{
						for(int i=0; i<p_mat->cols; i++)
						{
							float element = *(float *)CV_MAT_ELEM_PTR(*p_mat, j, i);
#ifdef DEBUG_DISPLAY_ARRAY
							printf("%f	", element);
#endif
						}
#ifdef DEBUG_DISPLAY_ARRAY
						printf("\n");
#endif
					}
				}
				else if(g_socket_decoding_result.index == INDEX_IMAGE)
				{
#ifdef DEBUG_DISPLAY_IMAGE
					cvShowImage("rx_image",g_socket_decoding_result.p_img);
					cvWaitKey(1);
#endif
				}
				ReleaseData_DecodingResultObj(&g_socket_decoding_result);	//释放空间
			}
			else if(result == ERR_BROKEN)
			{
				if(rx_num > RECV_BUF_SIZE)
				{
					printf("recv_buf_size is not enough!\n");
				}
				else
				{
					bytesRecv = recv(socket, // socket
						p_buf+rx_num, // 带偏移的接收缓存
						RECV_BUF_SIZE-rx_num, // 接收缓存变小	//简单设置下接收缓存
						0);// 标志
					printf("recv_byte=%d\n",bytesRecv);
					if(bytesRecv>0)
					{
						rx_num += bytesRecv;
						result = ERR_STICKING;
					}
					else
					{
						result = ERR_FAULT;
						printf("socket error!\n");
					}
				}
			}
		}
	}
}
*/

