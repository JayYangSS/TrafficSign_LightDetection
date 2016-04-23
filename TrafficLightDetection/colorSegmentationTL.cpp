#include "std_tlr.h"

#define SPECIAL_CASE 1 // "SPECIAL_CASE=0" means dealing with the lara video sequence


//
// function "colorSegmentation":
// color segmentatin using the hsi color model on the source image to extract the blob pixels 
//
IplImage* colorSegmentationTL(IplImage* inputImage)
{

	int colorB,colorG,colorR;
	int colorH,colorS,colorI;
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iInWidthStep = inputImage->widthStep;

	IplImage* imageSeg = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);
	//IplImage* imageSeg2 = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);

	if(!imageSeg)
		exit(EXIT_FAILURE);
	int iOutWidthStep = imageSeg->widthStep;//widthStep 表示存储一行像素所需的字节数

	unsigned char* in = (unsigned char*)inputImage->imageData;
	unsigned char* out =  (unsigned char*)imageSeg->imageData;

#if(SPECIAL_CASE)
	for(int j=0; j<iHeight; j++){
		in = (unsigned char*)inputImage->imageData + j*iInWidthStep;
		out = (unsigned char*)imageSeg->imageData + j*iOutWidthStep;
		if(j<=ROIHeight){
			for(int i=0; i<iWidth; i++){
				colorB = in[3*i];
				colorG = in[3*i+1];
				colorR = in[3*i+2];
				rgb2hsi(colorR,colorG,colorB,colorH,colorS,colorI);

				if (colorG >= 80 && (colorH >= 160 && colorH <= 200) && (colorS >= 30) && colorI >= 70)//2
				//if( colorR<=220 && (colorH>=140 && colorH<=195) && (colorS>=15) && colorI>=100 )//1
				//if( colorG>=70 && (colorH>=150 && colorH<=200) && (colorS>=13 && colorS<=100) && (colorI<=240&&colorI>=170) )
					out[i]=GREEN_PIXEL_LABEL;
				//else if( colorR>=200 && (colorH<=30 || colorH>=325) && colorS>=25 && colorI>=110)//1
				else if( colorR>=100&& (colorH<=30 || colorH>=240) && (colorS>=20&&colorS<130) && (colorI>=40&&colorI<170))
				//else if (colorR >= 200 && (colorH <= 30 || colorH >= 240) && (colorS >= 40 && colorS<80) && (colorI >= 120 && colorI<200))
					out[i]=RED_PIXEL_LABEL;
				else
					out[i]=NON_BLOB_PIXEL_LABEL;
			}
		}else{
			for(int i=0; i<iWidth; i++)
			{
				out[i]=NON_BLOB_PIXEL_LABEL;
			}		
	}
#endif
	}
	return imageSeg;
}




