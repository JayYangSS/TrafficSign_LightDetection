#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"
#include "math_utils.h"

void testY();
void test_nhs_Video();
void test_RBYcolor_Video();

int main()
{
	test_RBYcolor_Video();
	//test_nhs_Video();
	//testY();

	system("pause");

}



//检测黄色标志牌的黄色范围
void testY()
{
	char path[100];
	for (int i=0;i<44;i++)
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
	vector<Rect> boundingBox;
	Mat src,re_src,thresh;
	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign7.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		

		//TODO ADD

		Mat ihls_image = convert_rgb_to_ihls(re_src);
		Mat nhs_image = convert_ihls_to_nhs(ihls_image,1);//0:yellow,1:blue,2:red
		Mat noiseremove;
		imshow("ihls_image",ihls_image);
		waitKey(2);

		medianBlur(nhs_image,noiseremove,3);
		boundingBox.clear();
		//形状识别
		Mat p1=ShapeRecognize(nhs_image,boundingBox);
		/*Mat p2=ShapeRecognize(noiseremove);
		imshow("noise remove",p2);
		waitKey(5);*/
		for (int i=0;i<boundingBox.size();i++)
		{
			Point leftup(boundingBox[i].x,boundingBox[i].y);
			Point rightdown(boundingBox[i].x+boundingBox[i].width,boundingBox[i].y+boundingBox[i].height);
			rectangle(re_src,leftup,rightdown,CV_RGB(0,255,0),2);
		}
		
		imshow("re_src",re_src);
		waitKey(5);

		imshow("thresh img",p1);
		waitKey(5);


		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
}


//检测三角形，限速，蓝色标志牌的函数
void test_RBYcolor_Video()
{
	VideoCapture capture; 
	vector<Rect> boundingBox;
	Mat src,re_src,thresh;
	Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};

	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign7.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		
		Mat ihls_image = convert_rgb_to_ihls(re_src);
		//分别对黄蓝红颜色检测
		for (int mode=0;mode<3;mode++)
		{

			Mat nhs_image = convert_ihls_to_nhs(ihls_image,mode);//0:yellow,1:blue,2:red
			Mat noiseremove;
			//imshow("ihls_image",ihls_image);
			//waitKey(2);

			medianBlur(nhs_image,noiseremove,3);
			
			//形状识别
			//Mat p1=ShapeRecognize(nhs_image,boundingBox);
			Mat p2=ShapeRecognize(noiseremove,boundingBox);
			/*imshow("noise remove",p2);
			waitKey(5);*/
			
			//imshow("thresh img",p1);
			//waitKey(5);

			for (int i=0;i<boundingBox.size();i++)
			{
				Point leftup(boundingBox[i].x,boundingBox[i].y);
				Point rightdown(boundingBox[i].x+boundingBox[i].width,boundingBox[i].y+boundingBox[i].height);
				rectangle(re_src,leftup,rightdown,colorMode[mode],2);
			}
			boundingBox.clear();//必须清楚当前颜色的框，不然下一种颜色的框的起始位置就不是0了
			imshow("re_src",re_src);
			waitKey(5);
		}
		
		/*for (int i=0;i<boundingBox.size();i++)
		{
			Point leftup(boundingBox[i].x,boundingBox[i].y);
			Point rightdown(boundingBox[i].x+boundingBox[i].width,boundingBox[i].y+boundingBox[i].height);
			rectangle(re_src,leftup,rightdown,CV_RGB(0,255,0),2);
		}*/

		


		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
}