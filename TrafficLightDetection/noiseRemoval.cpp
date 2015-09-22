#include"std_tlr.h"

//
// function "noiseRemoval":
// applying the open morphology on the image of color segmentation 
//
IplImage* noiseRemoval(IplImage* inputImage)
{
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;

	IplImage* imageNoiseRem = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);
	if(!imageNoiseRem)
		exit(EXIT_FAILURE);

	IplConvKernel* structureEle1 =cvCreateStructuringElementEx(
		3,
		3,
		1,
		1,
		CV_SHAPE_ELLIPSE,
		0);

	int operationType[2] = {
		CV_MOP_OPEN,
		CV_MOP_CLOSE
	};

	//seems function open and close, as well as erode and dilate reversed. 
	cvMorphologyEx(inputImage,imageNoiseRem,NULL,structureEle1,operationType[0],1);
	//cvMorphologyEx(inputImage,imageNoiseRem,NULL,structureEle1,operationType[1],1);

	//in order to connect regions breaked by mophology operation ahead
	//cvErode(imageNoiseRem,imageNoiseRem,structureEle1,1);
	
	cvReleaseStructuringElement(&structureEle1);

	return imageNoiseRem;
}



//
// function "noiseRemoval2":
// applying the median smooth on the image of color segmentation 
//
IplImage* noiseRemoval2(IplImage* inputImage)
{
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;

	IplImage* imageNoiseRem = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,3);
	if(!imageNoiseRem)
		exit(EXIT_FAILURE);
	
	cvSmooth(inputImage,imageNoiseRem,CV_MEDIAN,3,3);	

	return imageNoiseRem;
}