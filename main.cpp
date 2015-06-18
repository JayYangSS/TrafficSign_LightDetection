#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"

int main()
{
	ClassifierTrain p;
	bool TRAIN=false;
	
	//red
	p.TrainSVM(TRAIN);
	
	

/*	VideoCapture capture; 
	Mat src,re_src;
	IplImage *thresh,*thres_res;
	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign6.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		thresh=&IplImage(re_src);
		thres_res=colorSegmentation(thresh);
		cvShowImage("seg",thres_res);
		cvWaitKey(5);

		imshow("src",re_src);
		waitKey(5);
		int end=cvGetTickCount();
		cvReleaseImage(&thres_res);

		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
	cvReleaseImage(&thresh);*/

	//Mat src=imread("shape.png",0);

	VideoCapture capture; 
	Mat src,re_src,thresh;
	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign6.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		thresh=p.colorThreshold(re_src);
		
		Mat tmp_thresh;
		thresh.convertTo(tmp_thresh,CV_8UC1,255);//必须转为CV_8UC1，不然后面的canny函数报错
		int g_nStructElementSize = 3; //结构元素(内核矩阵)的尺寸
		Mat noiseremove;
		Mat element = getStructuringElement(MORPH_RECT,Size(g_nStructElementSize+1,g_nStructElementSize+1),Point( g_nStructElementSize, g_nStructElementSize ));
		erode(tmp_thresh,noiseremove,element);//remove noise


		//形状识别
		Mat p1=ShapeRecognize(tmp_thresh);
		Mat p2=ShapeRecognize(noiseremove);
		imshow("noise remove",p2);
		waitKey(5);
		imshow("thresh img",p1);
		waitKey(5);


		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	

	system("pause");

}

