#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"
#include "math_utils.h"

void testY();
void test_nhs_Video();

int main()
{

	test_nhs_Video();
	//testY();

	system("pause");

}



//检测黄色标志牌的黄色范围
void testY()
{
	char path[100];
	for (int i=0;i<7;i++)
	{
		sprintf_s(path,"D:\\JY\\JY_TrainingSamples\\color\\yellow\\%d.jpg",i);
		Mat src=imread(path);
		Mat ihls=convert_rgb_to_ihls(src);
		Mat nhls=convert_ihls_to_nhs(ihls,0);
	}
}


//使用ihls->nhs的颜色检测+形状检测的测试视频程序
void test_nhs_Video()
{
	VideoCapture capture; 
	Mat src,re_src,thresh;
	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign3.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		imshow("re_src",re_src);
		waitKey(5);

		//显示直方图
		showHist(re_src);

		//TODO ADD

		Mat ihls_image = convert_rgb_to_ihls(re_src);
		Mat nhs_image = convert_ihls_to_nhs(ihls_image,0);//0:yellow,1:blue,2:red
		Mat noiseremove;
		imshow("ihls_image",ihls_image);
		waitKey(2);

		medianBlur(nhs_image,noiseremove,3);

		//形状识别
		Mat p1=ShapeRecognize(nhs_image);
		Mat p2=ShapeRecognize(noiseremove);
		imshow("noise remove",p2);
		waitKey(5);
		imshow("thresh img",p1);
		waitKey(5);


		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
}