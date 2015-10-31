#include "std_tlr.h"


bool rectangleDetection(IplImage* inputImage,IplImage* srcImage,CvRect iRect,int iColor,int* p1,int* p2)//p1为红灯，p2为绿灯
{
	
	/*int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iWidthStep = inputImage->widthStep; 
	IplImage* imageGrayScale = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);
	if(!imageGrayScale)
		exit(EXIT_FAILURE);
	cvCvtColor(srcImage,imageGrayScale,CV_BGR2GRAY);*/
	
	
	
	bool returnStatus = false;
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iWidthStep = inputImage->widthStep;
	int iSrcWidthStep = srcImage->widthStep;

	//thresholding for graylevel differences between seedpoints and its neibours
	 const int grayThresholding =80;//70
	 const int numThresholding =  50;//80

	 int iRectangleWidth;
	 int iRectangleHeight;

	 iRectangleWidth = (iRect.width+iRect.height)/2-1;
	 iRectangleHeight = iRectangleWidth*3;

	int iRectangleStartX=0,iRectangleEndX=0;
	int iRectangleStartY=0,iRectangleEndY=0;

	int iDrawRectWidth = (iRect.width+iRect.height)/2 + 6;
	int iDrawRectHeight = 3*(iDrawRectWidth-4)+6;
	int iDrawRectX1=0, iDrawRectY1=0;
	int iDrawRectX2=0, iDrawRectY2=0;

	if(iColor==RED_PIXEL_LABEL){
		//iRectangleStartY = iRect.y+1;
		iRectangleStartY = iRect.y + iRectangleHeight/3 +2;
		iRectangleEndY = iRect.y + iRectangleHeight +2;
		iRectangleStartX = iRect.x+1;
		iDrawRectY1 = iRect.y - 3;
	}
	else if(iColor == GREEN_PIXEL_LABEL){
		iRectangleStartY = iRect.y - iRectangleHeight/3*2- 2;
		iRectangleEndY = iRect.y - 2;
		iRectangleStartX = iRect.x;
		iDrawRectY1 = iRect.y-iDrawRectHeight/3*2;
	}

	//iRectangleEndY = iRectangleStartY + iRectangleHeight;
	//iRectangleStartX = iRect.x+1;
	iRectangleEndX = iRectangleStartX + iRectangleWidth;

	iDrawRectY2 = iDrawRectY1 + iDrawRectHeight;
	iDrawRectX1 = iRect.x-3;
	iDrawRectX2 = iDrawRectX1 + iDrawRectWidth;


	if( iRectangleStartX<0 || iRectangleStartY<0 || iRectangleEndX>=iWidth || iRectangleEndY>=iHeight)
		return returnStatus;

	int sum=0;
	int gray=0;
	int bValue=0,gValue=0,rValue=0;
	int bgrMax=0,bgrMin=0;
	unsigned char* pData;
	unsigned char* pSrcData;
	for(int j=iRectangleStartY; j<=iRectangleEndY; j++){
		pData = (unsigned char*)inputImage->imageData + j*iWidthStep;
		pSrcData = (unsigned char*)srcImage->imageData + j*iSrcWidthStep;
		for(int i=iRectangleStartX; i<=iRectangleEndX; i++){
			gray = pData[i];
			bValue = pSrcData[3*i]; gValue = pSrcData[3*i+1]; rValue = pSrcData[3*i+2];
			bgrMax = max(max(bValue,gValue),rValue);
			bgrMin = min(min(bValue,gValue),rValue);
			if((gray<=grayThresholding))
				sum++;
		}
	}	


	int ratio = (float)sum*100/(float)((iRectangleWidth+1)*((float)iRectangleHeight*2/3+1));//矩形框中黑色像素所占比例

	if(ratio>=numThresholding&&BlackAroundLight(srcImage,iRect))
		returnStatus = true;

	//若检测出的矩形框符合条件，则在原始图像上画出矩形标示框
	if(returnStatus==true)
	{
	    //在原始图像上画出矩形标示框
		if(iDrawRectX1<0)
			iDrawRectX1=0;
		if(iDrawRectY1<0)
			iDrawRectY1=0;
		if(iDrawRectX2>=iWidth)
			iDrawRectX2=iWidth-1;
		if(iDrawRectY2>=iHeight)
			iDrawRectY2=iHeight-1;

		if(iColor==GREEN_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,255,0),2);
			*p2=*p2+1;
		}

		else if(iColor==RED_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,0,255),2);
			*p1=*p1+1;
		}
	}
	 return returnStatus;
}