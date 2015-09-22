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

#if(!SPECIAL_CASE)
	for(int j=0; j<iHeight; j++){
		in = (unsigned char*)inputImage->imageData + j*iInWidthStep;
		out = (unsigned char*)imageSeg->imageData + j*iOutWidthStep;

		if(j<=ROIHeight)
			for(int i=0; i<iWidth; i++){
				if(i>=ROIWidth){
					colorB = in[3*i];
					colorG = in[3*i+1];
					colorR = in[3*i+2];
					rgb2hsi(colorR,colorG,colorB,colorH,colorS,colorI);
					
					if( colorR<=220 && (colorH>=140 && colorH<=195) && (colorS>=15 && colorS<=70) && colorI>=110 )
						out[i]=GREEN_PIXEL_LABEL;
					else if( colorB<=128 && (colorH<=75 || colorH>=348) && colorS>=30 && colorI>=85)
						out[i]=RED_PIXEL_LABEL;
					else
						out[i]=NON_BLOB_PIXEL_LABEL;
				}
				else
					out[i]=NON_BLOB_PIXEL_LABEL;

			}
		else
			for(int i=0; i<iWidth; i++){
				out[i]=NON_BLOB_PIXEL_LABEL;
			}

	}
#endif


#if(SPECIAL_CASE)
	for(int j=0; j<iHeight; j++){
		in = (unsigned char*)inputImage->imageData + j*iInWidthStep;
		out = (unsigned char*)imageSeg->imageData + j*iOutWidthStep;
		if(j<=ROIHeight)
			for(int i=0; i<iWidth; i++){
				colorB = in[3*i];
				colorG = in[3*i+1];
				colorR = in[3*i+2];
				rgb2hsi(colorR,colorG,colorB,colorH,colorS,colorI);
			    
				//if( colorR<=220 && (colorH>=140 && colorH<=195) && (colorS>=15 && colorS<=70) && colorI>=110 )
				if( colorG>=70 && (colorH>=150 && colorH<=200) && (colorS>=13 && colorS<=100) && (colorI<=240&&colorI>=170) )
					out[i]=GREEN_PIXEL_LABEL;
				//else if( colorR>=200 && (colorH<=30 || colorH>=325) && colorS>=25 && colorI>=110)
				else if( colorR>=120&& (colorH<=60 || colorH>=350) && (colorS>=20&&colorS<130) && (colorI>=70&&colorI<220))
					out[i]=RED_PIXEL_LABEL;
				else
					out[i]=NON_BLOB_PIXEL_LABEL;
			}
	}
#endif

	return imageSeg;
}




