#include "std_tlr.h"

void GetImageRect(IplImage* orgImage, CvRect rectInImage, IplImage* imgRect)
{
	//从图像orgImage中提取一块（rectInImage）子图像imgRect
	IplImage *result=imgRect;
	CvSize size;
	size.width=rectInImage.width;
	size.height=rectInImage.height;

	//从图像中提取子图像
	cvSetImageROI(orgImage,rectInImage);
	cvCopy(orgImage,result);
	cvResetImageROI(orgImage);
}


bool isLighInBox(Mat src)
{
	//TODO:这里的处理逻辑还没有想好
	Mat topHat;
	Mat kernal=getStructuringElement(MORPH_ELLIPSE,Size(5,5));
	morphologyEx(src,topHat,MORPH_TOPHAT,kernal,Point(-1,-1),4);
	double max_topHat,min_topHat;
	Point maxPoint,minPoint;
	minMaxLoc(topHat,&min_topHat,&max_topHat,&minPoint,&maxPoint);
	imshow("src",src);
	waitKey(1);
	imshow("topHat",topHat);
	waitKey(1);
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

void rectangleDetection(IplImage* inputImage,IplImage* srcImage,CvRect iRect,int iColor,int* p1,int* p2)//p1为前行位，p2为左转位
{
	const int iWidth = inputImage->width;
	const int iHeight = inputImage->height;
	
	//水平和竖直状态
	bool VerticalReturnStatus = false;
	bool HorzReturnStatus=false;

	//横向检测框
	int HorzRectHeight=(iRect.width+iRect.height)/2 + 6;
	int HorzRectWidth=3*(HorzRectHeight-4)+3;
	int HorzRectX1=0, HorzRectY1=0;
	int HorzRectX2=0, HorzRectY2=0;


	//thresholding for graylevel differences between seedpoints and its neibours
	const int grayThresholding =80;//70
	const int RatioThreshold =  55;//检测框中黑色像素所占比例

	//纵向检测框
	int iDrawRectWidth = (iRect.width+iRect.height)/2 + 6;
	int iDrawRectHeight = 3*(iDrawRectWidth-4)+6;
	int iDrawRectX1=0, iDrawRectY1=0;
	int iDrawRectX2=0, iDrawRectY2=0;

	if(iColor==RED_PIXEL_LABEL){
		iDrawRectY1 = iRect.y - 3;
		HorzRectX1= iRect.x-3;
	}
	else if(iColor == GREEN_PIXEL_LABEL){
		iDrawRectY1 = iRect.y-iDrawRectHeight/3*2;
		HorzRectX1=iRect.x-HorzRectWidth/3*2;
	}

	//竖直检测窗设置
	iDrawRectY2 = iDrawRectY1 + iDrawRectHeight;
	iDrawRectX1 = iRect.x-3;
	iDrawRectX2 = iDrawRectX1 + iDrawRectWidth;

	//水平检测框设置
	HorzRectX2= HorzRectX1+HorzRectWidth;
	HorzRectY1= iRect.y-3;
	HorzRectY2= HorzRectY1+HorzRectHeight;

	if(HorzRectX1<0 || HorzRectY1<0 || HorzRectX2>=iWidth || HorzRectY2>=iHeight)
	{
		//cvReleaseImage(&imageGrayScale);//when return the result, the image must be released, otherwise,the memory will be leaked
		return;
	}
	
	if( iDrawRectX1<0 || iDrawRectY1<0 || iDrawRectX2>=iWidth || iDrawRectY2>=iHeight)
	{
		//cvReleaseImage(&imageGrayScale);//when return the result, the image must be released, otherwise,the memory will be leaked
		return;
	}


	//竖直方向统计黑色像素比例
	CvRect VerticalRect;
	VerticalRect.x=iDrawRectX1;
	VerticalRect.y=iDrawRectY1;
	VerticalRect.width=iDrawRectWidth;
	VerticalRect.height=iDrawRectHeight;
	IplImage*VerticalLight = cvCreateImage(cvSize(iDrawRectWidth,iDrawRectHeight),srcImage->depth,srcImage->nChannels);
	GetImageRect(srcImage,VerticalRect,VerticalLight);
	IplImage *VerticalGrayLight=cvCreateImage(cvSize(iDrawRectWidth,iDrawRectHeight),IPL_DEPTH_8U,1);
	cvCvtColor(VerticalLight,VerticalGrayLight,CV_BGR2GRAY);
	cvThreshold(VerticalGrayLight,VerticalGrayLight,0,255,CV_THRESH_OTSU);


	/*
	int iWidthStep = VerticalGrayLight->widthStep; 
	int sum=0;
	int VerticalGrayValue=0;
	unsigned char* pDataVertical;
	for(int j=0; j<iDrawRectHeight; j++){
		pDataVertical = (unsigned char*)VerticalGrayLight->imageData + j*iWidthStep;
		for(int i=0; i<iDrawRectWidth; i++){
			VerticalGrayValue = pDataVertical[i];
			if((VerticalGrayValue<=grayThresholding))
				sum++;
		}
	}*/	

	int cvVerticalSum=cvCountNonZero(VerticalGrayLight);
	int verticalBlackNum=iDrawRectWidth*iDrawRectHeight-cvVerticalSum;//黑色像素点个数
	cvReleaseImage(&VerticalLight);
	cvReleaseImage(&VerticalGrayLight);



	//水平方向统计黑色像素比例
	CvRect HorzRect;
	HorzRect.x=HorzRectX1;
	HorzRect.y=HorzRectY1;
	HorzRect.width=HorzRectWidth;
	HorzRect.height=HorzRectHeight;
	IplImage*HorzLight = cvCreateImage(cvSize(HorzRectWidth,HorzRectHeight),srcImage->depth,srcImage->nChannels);
	GetImageRect(srcImage,HorzRect,HorzLight);
	IplImage *HorzGrayLight=cvCreateImage(cvSize(HorzRectWidth,HorzRectHeight),IPL_DEPTH_8U,1);
	cvCvtColor(HorzLight,HorzGrayLight,CV_BGR2GRAY);
	cvThreshold(HorzGrayLight,HorzGrayLight,0,255,CV_THRESH_OTSU);
	
	
/*	
	int HorzWidthStep = HorzGrayLight->widthStep; 
	int HorzSum=0;
	int HorzGrayValue=0;
	unsigned char* pDataHorz;
	for(int j=0; j<HorzRectHeight; j++){
		pDataHorz = (unsigned char*)HorzGrayLight->imageData + j*HorzWidthStep;
		for(int i=0; i<HorzRectWidth; i++){
			HorzGrayValue = pDataHorz[i];
			//if((HorzGrayValue<=grayThresholding))
			if((HorzGrayValue==0))
				HorzSum++;
		}
	}	*/
	int cvHorzSum=cvCountNonZero(HorzGrayLight);
	int horzBlackNum=HorzRectWidth*HorzRectHeight-cvHorzSum;
	cvReleaseImage(&HorzLight);
	cvReleaseImage(&HorzGrayLight);
	

	int VerticalBlackRatio = (float)verticalBlackNum*100/(float)((iDrawRectWidth+1)*((float)iDrawRectHeight+1));//矩形框中黑色像素所占比例
	int HorzBlackRatio=(float)horzBlackNum*100/(float)((HorzRectWidth+1)*((float)HorzRectHeight+1));//矩形框中黑色像素所占比例
	
#if ISDEBUG_TL
	ofstream outfile;
	outfile.open(debugTLPath,ios::app);//ios::app： 以追加的方式打开文件
	outfile<<"===black VerticalBlackRatio===:"<<VerticalBlackRatio<<endl;//输出到调试文件中
	cout<<"===black VerticalBlackRatio===:"<<VerticalBlackRatio<<endl;//输出到控制台
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

	int DetectResult=isTL(srcImage,iRect);
	if (DetectResult==1)
	{
		//cout<<"Horz Ratio:"<<HorzBlackRatio<<endl;
		//cout<<"Vertical Ratio:"<<VerticalBlackRatio<<endl;
		if(VerticalBlackRatio>=RatioThreshold&&VerticalBlackRatio<=93)
			VerticalReturnStatus = true;
		else if (HorzBlackRatio>=RatioThreshold&&HorzBlackRatio<=90)
		{
			HorzReturnStatus=true;
		}
	}
	 

	//若检测出的矩形框符合条件，则在原始图像上画出矩形标示框
	if(VerticalReturnStatus==true)
	{
		if(iColor==GREEN_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,255,0),2);
			//*p2=*p2+1;
		}

		else if(iColor==RED_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,0,255),2);
			//*p1=*p1+1;


			//TODO:识别信号灯指向
			int result=RecognizeLight(srcImage,iRect);
			switch(result)
			{
			case 0://圆形
				//cout<<"圆形"<<endl;
				*p1=1;
				break;
			case 1://禁止左转
				//cout<<"禁止左转"<<endl;
				*p2=1;
				break;
			case 2://前行箭头
				//cout<<"禁止右转"<<endl;
				*p1=1;
				break;
			default:
				break;
			}
		}
	}else if (HorzReturnStatus)
	{
		//横向检测
		if(iColor==GREEN_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(HorzRectX1,HorzRectY1),cvPoint(HorzRectX2,HorzRectY2),cvScalar(0,255,0),2);
			//*p2=*p2+1;
		}

		else if(iColor==RED_PIXEL_LABEL)
		{
			cvRectangle(srcImage,cvPoint(HorzRectX1,HorzRectY1),cvPoint(HorzRectX2,HorzRectY2),cvScalar(0,0,255),2);
			//*p1=*p1+1;


			//TODO:识别信号灯指向
			int result=RecognizeLight(srcImage,iRect);
			switch(result)
			{
			case 0://圆形
				//cout<<"圆形"<<endl;
				*p1=1;
				break;
			case 1://禁止左转
				//cout<<"禁止左转"<<endl;
				*p2=1;
				break;
			case 2://前行箭头
				//cout<<"禁止右转"<<endl;
				*p1=1;
				break;
			default:
				break;
			}
		}
	}
	return;
}