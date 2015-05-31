
#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"

int main()
{
	/*vector<Rect> found_filtered;
	bool TRAIN=true;
	int bytes_recv = 0;
	


	HOGDescriptor myHOG(Size(20,20),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
	Mat src,temp,re_src;
	deque<int> resultR_static,resultN_static;
	VideoCapture capture; 

	//train
	hogSVMTrain(myHOG,TRAIN);


	//start 
	capture.open("D:\\JY\\JY_TrainingSamples\\light2.avi");
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
	return 0;*/

	ClassifierTrain p;
	//vector<PixelRGB> rgb;
	Mat p1=imread("D:\\JY\\JY_TrainingSamples\\color\\positive\\1.jpg");
	Mat p2=imread("D:\\JY\\JY_TrainingSamples\\color\\positive\\2.jpg");
	Mat p3=imread("D:\\JY\\JY_TrainingSamples\\color\\positive\\3.jpg");
	vector<Mat> pos;
	pos.push_back(p1);pos.push_back(p2);pos.push_back(p3);



	Mat n1=imread("D:\\JY\\JY_TrainingSamples\\color\\negative\\1.jpg");
	Mat n2=imread("D:\\JY\\JY_TrainingSamples\\color\\negative\\2.jpg");
	Mat n3=imread("D:\\JY\\JY_TrainingSamples\\color\\negative\\3.jpg");
	vector<Mat> neg;
	neg.push_back(n1);neg.push_back(n2);neg.push_back(n3);
	p.getRGB(pos,neg);


	MySVM svm;
	p.train();
	//Mat test=imread("D:\\JY\\JY_TrainingSamples\\TrafficSign\\1.jpg");
	Mat test=imread("D:\\JY\\JY_TrainingSamples\\TestIJCNN2013\\TestIJCNN2013Download\\00004.ppm");
	Mat result=p.colorThreshold(test);
	imshow("result",result);
	waitKey();
	system("pause");
}