#include "traffic.h"





void showHist(Mat src)
{
	Mat  hsv_src,dst;

	if( !src.data )
		cout<<"The image data is null!"<<endl;

	/// 分割成3个单通道图像 ( R, G 和 B )
	vector<Mat> hsv_planes;
	cvtColor(src,hsv_src,CV_BGR2HSV);
	split( hsv_src, hsv_planes );

	/// 设定bin数目
	int histSize = 255;

	/// 设定取值范围 ( R,G,B) )
	float range[] = { 0, 255 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	Mat h_hist, s_hist, v_hist;

	/// 计算直方图:
	calcHist( &hsv_planes[0], 1, 0, Mat(), h_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &hsv_planes[1], 1, 0, Mat(), s_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &hsv_planes[2], 1, 0, Mat(), v_hist, 1, &histSize, &histRange, uniform, accumulate );

	// 创建直方图画布
	int hist_w = 400; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );

	Mat histImage( hist_w, hist_h, CV_8UC3, Scalar( 0,0,0) );

	/// 将直方图归一化到范围 [ 0, histImage.rows ]
	normalize(h_hist, h_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(s_hist, s_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(v_hist, v_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	/// 在直方图画布上画出直方图
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(h_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(h_hist.at<float>(i)) ),
			Scalar( 0, 0, 255), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(s_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(s_hist.at<float>(i)) ),
			Scalar( 0, 255, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(v_hist.at<float>(i-1)) ) ,
			Point( bin_w*(i), hist_h - cvRound(v_hist.at<float>(i)) ),
			Scalar( 255, 0, 0), 2, 8, 0  );
	}

	/// 显示直方图
	namedWindow("calcHist Demo", CV_WINDOW_AUTOSIZE );
	imshow("calcHist Demo", histImage );

	waitKey(5);

}






void rgb2hsi(int red, int green, int blue, int& hue, int& saturation, int& intensity )
{
	double r,g,b;
	double h,s,i;

	double sum;
	double minRGB,maxRGB;
	double theta;

	r = red/255.0;
	g = green/255.0;
	b = blue/255.0;

	minRGB = ((r<g)?(r):(g));
	minRGB = (minRGB<b)?(minRGB):(b);

	maxRGB = ((r>g)?(r):(g));
	maxRGB = (maxRGB>b)?(maxRGB):(b);

	sum = r+g+b;
	i = sum/3.0;

	if( i<0.001 || maxRGB-minRGB<0.001 )
	{
		//this is a black image or grayscale image
		//in this circumstance, hue is undefined, not zero
		h=0.0;
		s=0.0;
		//return ;
	}
	else
	{
		s = 1.0-3.0*minRGB/sum;
		theta = sqrt((r-g)*(r-g)+(r-b)*(g-b));
		theta = acos((r-g+r-b)*0.5/theta);
		if(b<=g)
			h = theta;
		else 
			h = 2*PI - theta;
		if(s<=0.01)
			h=0;
	}

	hue = (int)(h*180/PI);
	saturation = (int)(s*100);
	intensity = (int)(i*255);
}

IplImage* colorSegmentation(IplImage* inputImage)
{

	int colorB,colorG,colorR;
	int colorH,colorS,colorI;
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iInWidthStep = inputImage->widthStep;
	int Green_num=0,Red_num=0;

	IplImage* imageSeg = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);

	if(!imageSeg)
		exit(EXIT_FAILURE);
	int iOutWidthStep = imageSeg->widthStep;//widthStep ±íÊ¾´æ´¢Ò»ÐÐÏñËØËùÐèµÄ×Ö½ÚÊý

	unsigned char* in = (unsigned char*)inputImage->imageData;
	unsigned char* out =  (unsigned char*)imageSeg->imageData;


	for(int j=0; j<iHeight; j++)
	{
		in = (unsigned char*)inputImage->imageData + j*iInWidthStep;
		out = (unsigned char*)imageSeg->imageData + j*iOutWidthStep;
		if(j<=ROIHeight)
			for(int i=0; i<iWidth; i++){
				colorB = in[3*i];
				colorG = in[3*i+1];
				colorR = in[3*i+2];
				rgb2hsi(colorR,colorG,colorB,colorH,colorS,colorI);
			    
			//下面的参数参考论文：	A Method to Search for Color Segmentation Threshold in Traffic Sign Detection
				if((colorR-colorG)<150&&(colorR-colorG)>30&&(colorG-colorB)<50)
				{
					out[i]=GREEN_PIXEL_LABEL;
					Green_num++;
				}
				
				else if((colorR-colorG<75)&&(colorG-colorB)<120)
				{
					out[i]=NON_BLOB_PIXEL_LABEL;
					Red_num++;
				}
					
				else
					out[i]=RED_PIXEL_LABEL;
			}
	}
	
/*
	if((Green_num>Red_num)&&(Green_num>10))
		return RESULT_G;
	else if((Green_num<Red_num)&&(Red_num>10))
		return RESULT_R;
	else
		return RESULT_NON;*/
	return imageSeg;

}
