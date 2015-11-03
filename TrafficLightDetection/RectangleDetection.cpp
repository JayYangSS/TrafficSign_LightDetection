#include "std_tlr.h"

bool isLighInBox(Mat src)
{
	//TODO:这里的处理逻辑还没有想好
	Mat topHat;
	Mat kernal=getStructuringElement(MORPH_ELLIPSE,Size(5,5));
	morphologyEx(src,topHat,MORPH_TOPHAT,kernal,Point(-1,-1),4);
	double max_topHat,min_topHat;
	Point maxPoint,minPoint;
	minMaxLoc(topHat,&min_topHat,&max_topHat,&minPoint,&maxPoint);
	

	double max_tempGray,min_tempGray;
	Point maxPoint_tempGray,minPoint_tempGray;
	minMaxLoc(src,&min_tempGray,&max_tempGray,&minPoint_tempGray,&maxPoint_tempGray);

#if ISDEBUG_TL
	cout<<"min_topHat:"<<min_topHat<<endl;
	cout<<"max_topHat:"<<max_topHat<<endl;
	cout<<"min_gray:"<<min_tempGray<<endl;
	cout<<"max_gray:"<<max_tempGray<<endl;

	ofstream outfile;
	outfile.open(debugTLPath,ios::app);
	outfile<<"min_topHat:"<<min_topHat<<endl;
	outfile<<"max_topHat:"<<max_topHat<<endl;
	outfile<<"min_gray:"<<min_tempGray<<endl;
	outfile<<"max_gray:"<<max_tempGray<<endl;
	outfile<<""<<endl;
	outfile.close();
#endif
	
	return false;
}

bool rectangleDetection(IplImage* inputImage,IplImage* srcImage,CvRect iRect,int iColor,int* p1,int* p2)//p1为红灯，p2为绿灯
{
	
	const int iWidth = inputImage->width;
	const int iHeight = inputImage->height;
	int iWidthStep = inputImage->widthStep; 
	IplImage* imageGrayScale = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);
	cvCvtColor(srcImage,imageGrayScale,CV_BGR2GRAY);
	
	
	bool returnStatus = false;
	int iSrcWidthStep = srcImage->widthStep;

	//thresholding for graylevel differences between seedpoints and its neibours
	const int grayThresholding =70;//70
	const int RatioThreshold =  55;//检测框中黑色像素所占比例

	int iDrawRectWidth = (iRect.width+iRect.height)/2 + 6;
	int iDrawRectHeight = 3*(iDrawRectWidth-4)+6;
	int iDrawRectX1=0, iDrawRectY1=0;
	int iDrawRectX2=0, iDrawRectY2=0;

	if(iColor==RED_PIXEL_LABEL){
		iDrawRectY1 = iRect.y - 3;
	}
	else if(iColor == GREEN_PIXEL_LABEL){
		iDrawRectY1 = iRect.y-iDrawRectHeight/3*2;
	}

	iDrawRectY2 = iDrawRectY1 + iDrawRectHeight;
	iDrawRectX1 = iRect.x-3;
	iDrawRectX2 = iDrawRectX1 + iDrawRectWidth;


	if( iDrawRectX1<0 || iDrawRectY1<0 || iDrawRectX2>=iWidth || iDrawRectY2>=iHeight)
		return returnStatus;

	int sum=0;
	int grayValue=0;
	int bValue=0,gValue=0,rValue=0;
	int bgrMax=0,bgrMin=0;
	unsigned char* pData;
	unsigned char* pSrcData;
	for(int j=iDrawRectY1; j<=iDrawRectY2; j++){
		pData = (unsigned char*)imageGrayScale->imageData + j*iWidthStep;
		pSrcData = (unsigned char*)srcImage->imageData + j*iSrcWidthStep;
		for(int i=iDrawRectX1; i<=iDrawRectX2; i++){
			grayValue = pData[i];
		//	bValue = pSrcData[3*i]; gValue = pSrcData[3*i+1]; rValue = pSrcData[3*i+2];
		//	bgrMax = max(max(bValue,gValue),rValue);
		//	bgrMin = min(min(bValue,gValue),rValue);
			if((grayValue<=grayThresholding))
				sum++;
		}
	}	

	iDrawRectHeight=iDrawRectY2-iDrawRectY1;
	iDrawRectWidth=iDrawRectX2-iDrawRectX1;

	int ratio = (float)sum*100/(float)((iDrawRectWidth+1)*((float)iDrawRectHeight+1));//矩形框中黑色像素所占比例
	
#if ISDEBUG_TL
	ofstream outfile;
	outfile.open(debugTLPath,ios::app);//ios::app： 以追加的方式打开文件
	outfile<<"===black ratio===:"<<ratio<<endl;//输出到调试文件中
	cout<<"===black ratio===:"<<ratio<<endl;//输出到控制台
	outfile.close();
#endif

#if ISDEBUG_TL
	Mat grayMat(imageGrayScale);
	Rect drawRect;
	drawRect.x=iDrawRectX1;
	drawRect.y=iDrawRectY1;
	drawRect.width=iDrawRectX2-iDrawRectX1;
	drawRect.height=iDrawRectY2-iDrawRectY1;
	Mat tmpMat=grayMat(drawRect);
	isLighInBox(tmpMat);
#endif

	if(ratio>=RatioThreshold&&BlackAroundLight(srcImage,iRect))
		returnStatus = true;

	//若检测出的矩形框符合条件，则在原始图像上画出矩形标示框
	if(returnStatus==true)
	{
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




	cvReleaseImage(&imageGrayScale);
	return returnStatus;
}