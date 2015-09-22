#ifndef STD_TLR_H
#define STD_TLR_H

#include "traffic.h"

#pragma comment(lib,"ws2_32.lib")
#pragma warning(disable:4819)

#include<WinSock2.h>//放最前面
#include<opencv2\opencv.hpp>
#include<iostream>
#include<fstream>
#include<sstream>
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<math.h>
#include <time.h>
#include <tchar.h>
#include <string>

#define TEST 0
#define PI 3.1415
#define GREEN_PIXEL_LABEL 255
#define RED_PIXEL_LABEL 128
#define NON_BLOB_PIXEL_LABEL 0
#define ROIHeight 500
#define ROIWidth 0
#define PosSamNO    523 //正样本个数
#define HORZ_PosSamNO    36 //正样本个数
#define NegSamNO 2574   //负样本个数
#define HORZ_NegSamNO 2416
#define HardExampleNO 334
#define HORZ_HardExampleNO 23

void rgb2hsi(int red, int green, int blue, int& hue, int& saturation, int& intensity );
void componentExtraction(IplImage* inputImage, IplImage* srcImage,int* p,vector<Rect> &found_filtered);
void hogSVMTrainTL(HOGDescriptor &myHOG,bool TRAIN,bool HORZ);
IplImage* HSV_view(IplImage* inputImage,int flagHSV);
IplImage* colorSegmentationTL(IplImage* inputImage);
IplImage* topHatExtraction(IplImage* inputImage);
IplImage* noiseRemoval(IplImage* inputImage);
IplImage* noiseRemoval2(IplImage* inputImage);
IplImage* fixImage(IplImage* colorseg,IplImage* tophat);
bool sizeFiltering(CvRect rect);
bool regionGrowFiltering(IplImage* inputImage,IplImage*colorSeg,CvRect iRect,CvRect& oRect,vector<Rect> &found_filtered);
bool rectangleDetection(IplImage* inputImage,IplImage* srcImage,CvRect iRect,int iColor,int* p1,int* p2);
bool BoxDetectTL(Mat src_test,HOGDescriptor &myHOG,vector<Rect> &found_filtered,bool HORZ);

using namespace std;
using namespace cv;

#endif