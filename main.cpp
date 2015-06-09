#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"

int main()
{
	ClassifierTrain p;
	bool TRAIN=true;
	
	//red
	p.TrainSVM(TRAIN);
	//Mat test=imread("D:\\JY\\JY_TrainingSamples\\TrafficSign\\1.jpg");
	Mat test=imread("D:\\JY\\JY_TrainingSamples\\TestIJCNN2013\\TestIJCNN2013Download\\00004.ppm");
	Mat result=p.colorThreshold(test);
	imshow("result",result);
	waitKey();

	system("pause");
}




