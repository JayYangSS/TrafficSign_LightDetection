
#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"
Mat shapeMatch(Mat src,Mat templ,Mat test,int match_method)
{
	int result_cols =  src.cols - templ.cols + 1;
	int result_rows = src.rows - templ.rows + 1;


	Mat result;
	result.create( result_cols, result_rows, CV_32FC1 );
	matchTemplate(src,templ,result,match_method);
	normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );

	double minVal; double maxVal; Point minLoc; Point maxLoc;
	Point matchLoc;

	minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );
	if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
	{ matchLoc = minLoc; }
	else
	{ matchLoc = maxLoc; }

	/// 让我看看您的最终结果
	//rectangle( src, matchLoc, Point( matchLoc.x + templ.cols , matchLoc.y + templ.rows ), Scalar::all(0), 2, 8, 0 );
	rectangle( test, matchLoc, Point( matchLoc.x + templ.cols , matchLoc.y + templ.rows ), Scalar::all(0), 2, 8, 0 );

	return test;
}


int main()
{
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
	Mat test=imread("D:\\JY\\JY_TrainingSamples\\TrafficSign\\1.jpg");
	//Mat test=imread("D:\\JY\\JY_TrainingSamples\\TestIJCNN2013\\TestIJCNN2013Download\\00004.ppm");
	Mat result=p.colorThreshold(test);
	imshow("result",result);
	waitKey();

	Mat templ=imread("D:\\JY\\TrafficSignDetection\\TrafficSignDetection\\template\\round.jpg",CV_LOAD_IMAGE_GRAYSCALE);
	result.convertTo(result,CV_8UC1);//MatchTemplate函数要求两张图片的type相同
	Mat match=shapeMatch(result,templ,test,CV_TM_SQDIFF_NORMED);
	imshow("matchResult",match);
	waitKey();

	system("pause");
}



