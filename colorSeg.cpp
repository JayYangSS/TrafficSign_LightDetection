#include "traffic.h"

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
