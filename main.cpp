#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"


int main()
{
	vector<Rect> found_filtered;
	bool TRAIN=true;
	int bytes_recv = 0;
	


	HOGDescriptor myHOG(Size(20,20),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
	Mat src,temp,re_src;
//	deque<int> resultR_static,resultN_static;//用来滤波，使结果更稳定准确
	VideoCapture capture; 

	//train
	hogSVMTrain(myHOG,TRAIN);


	//start 
	capture.open("D:\\JY\\JY_TrainingSamples\\huanhu_clip2.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		found_filtered.clear();
		BoxDetect(re_src,myHOG,found_filtered);
		int end=cvGetTickCount();


		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	

	system("pause");
	return 0;

}



