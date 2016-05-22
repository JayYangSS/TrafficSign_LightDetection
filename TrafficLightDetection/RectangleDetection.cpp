#include "std_tlr.h"

//reference paper:Suspend Traffic Lights Detection and Distance Estimation Using Color Features
//when a sufficient color pixel density is found, black pixel density is computed inside the other two
//corresponding blocks,the other two blocks must be almost dark.
bool checkOtherBlocksBlackRatio(IplImage* TLImg, int iColor, bool isVertical){
	
	int height = TLImg->height;
	int width = TLImg->width;
	int widthStep = TLImg->widthStep;
	int totalNum = height*width;
	
	//cvShowImage("TLImg", TLImg);
	//cvWaitKey(5);
	//vertical
	if (isVertical){
		//process the green light
		if (iColor == GREEN_PIXEL_LABEL){
			int j = 0;
			int greenBlackNum1 = 0;
			int greenBlackNum2 = 0;

			//calculate the first block
			for (; j < height / 3; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 0; i < width; i++){
					if (in[i] == 0)greenBlackNum1++;
				}
			}
			float blackRatio1 = (float)(greenBlackNum1 * 3) / (float)totalNum;
			if (blackRatio1 <= 0.5)return false;

			//calculate the second block
			for (; j < height *2/ 3; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 0; i < width; i++){
					if (in[i] == 0)greenBlackNum2++;
				}
			}
			float blackRatio2 = (float)(greenBlackNum2 * 3) / (float)totalNum;

			if (blackRatio2>0.5)return true;
			else
				return false;
		}
		//process the red light
		else{
			int j = height / 3;
			int redBlackNum1 = 0;
			int redBlackNum2 = 0;

			//calculate the first block
			for (; j < height *2/ 3; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 0; i < width; i++){
					if (in[i] == 0)redBlackNum1++;
				}
			}
			float blackRatio1 = (float)(redBlackNum1 * 3) / (float)totalNum;
			if (blackRatio1 <= 0.5)return false;

			//calculate the second block
			for (; j < height ; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 0; i < width; i++){
					if (in[i] == 0)redBlackNum2++;
				}
			}
			float blackRatio2 = (float)(redBlackNum2 * 3) / (float)totalNum;

			if (blackRatio2>0.5)return true;
			else
				return false;
		}
	}
	//horizental
	else{
		//process the green light
		if (iColor == GREEN_PIXEL_LABEL){
			int greenBlackNum1 = 0;
			int greenBlackNum2 = 0;

			//calculate the first block
			for (int j=0; j < height ; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 0; i < width/3; i++){
					if (in[i] == 0)greenBlackNum1++;
				}
			}
			float blackRatio1 = (float)(greenBlackNum1 * 3) / (float)totalNum;
			if (blackRatio1 <= 0.6)return false;
			//calculate the second block
			for (int j = 0; j < height ; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = width/3; i < width*2/3; i++){
					if (in[i] == 0)greenBlackNum2++;
				}
			}
			float blackRatio2 = (float)(greenBlackNum2 * 3) / (float)totalNum;

			if (blackRatio2>0.7)return true;
			else
				return false;
		}
		//process the red light
		else{
			int redBlackNum1 = 0;
			int redBlackNum2 = 0;

			//calculate the first block
			for (int j=0; j < height ; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = width/3; i < 2*width/3; i++){
					if (in[i] == 0)redBlackNum1++;
				}
			}
			float blackRatio1 = (float)(redBlackNum1 * 3) / (float)totalNum;
			if (blackRatio1 <= 0.5)return false;
			//calculate the second block
			for (int j=0; j < height; j++){
				uchar* in = (uchar*)TLImg->imageData + j* widthStep;
				for (int i = 2*width/3; i < width; i++){
					if (in[i] == 0)redBlackNum2++;
				}
			}
			float blackRatio2 = (float)(redBlackNum2 * 3) / (float)totalNum;

			if (blackRatio2>0.5)return true;
			else
				return false;
		}
	}
}



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


void rectangleDetection(IplImage* inputImage,IplImage* srcImage,CvRect iRect,int iColor,vector<ShapeRecResult> &v)//p1为前行位，p2为左转位
{
	const int iWidth = inputImage->width;
	const int iHeight = inputImage->height;
	
	//水平和竖直状态
	//bool VerticalReturnStatus = false;
	//bool HorzReturnStatus=false;

	//横向检测框
	int HorzRectHeight=(iRect.width+iRect.height)/2 + 6;
	int HorzRectWidth=3*(HorzRectHeight-4)+3;
	int HorzRectX1=0, HorzRectY1=0;
	int HorzRectX2=0, HorzRectY2=0;


	//thresholding for graylevel differences between seedpoints and its neibours
	const int grayThresholding =80;//70
	const int RatioThreshold =  55;//检测框中黑色像素所占比例

	//纵向检测框
	//int iDrawRectWidth = (iRect.width+iRect.height)/2 + 6;
	int iDrawRectWidth = (iRect.width + iRect.height)/2 *5/ 3;
	//int iDrawRectHeight = 3*(iDrawRectWidth-4)+6;
	int iDrawRectHeight;
	int iDrawRectX1=0, iDrawRectY1=0;
	int iDrawRectX2=0, iDrawRectY2=0;

	if(iColor==RED_PIXEL_LABEL){
		iDrawRectHeight = iDrawRectWidth * 7 / 3;
		iDrawRectY1 = iRect.y - iDrawRectWidth /4;
		HorzRectX1= iRect.x-3;
		///iDrawRectHeight = iDrawRectWidth * 7 / 3;
	}
	else if(iColor == GREEN_PIXEL_LABEL){
		iDrawRectHeight = iDrawRectWidth * 8 / 3;
		iDrawRectY1 = iRect.y-iDrawRectHeight/3*2;
		HorzRectX1=iRect.x-HorzRectWidth/3*2;
	}

	//竖直检测窗设置
	iDrawRectY2 = iDrawRectY1 + iDrawRectHeight;
	iDrawRectX1 = iRect.x - iDrawRectWidth/5;
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
	
	//get the other two  blocks black ration (vertical)
	bool verticalBlackLimit = checkOtherBlocksBlackRatio(VerticalGrayLight, iColor,true);

	ShapeRecResult TLbox;
	//若检测出的矩形框符合条件，则将坚持到的矩形框放入v中，在外面统一显示
	//if(VerticalBlackRatio>=RatioThreshold&&VerticalBlackRatio<=93)
	//if (verticalBlackLimit&&isTL(srcImage, VerticalRect, true)){
	if (verticalBlackLimit == true && isTL(srcImage, VerticalRect, true))
	{
		TLbox.box = VerticalRect;
		//TLbox.shape = 1;//表示竖向

		if (iColor == GREEN_PIXEL_LABEL)
		{
			//cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,255,0),2);
			TLbox.color = GREEN_PIXEL_LABEL;
			v.push_back(TLbox);
		}

		else if (iColor == RED_PIXEL_LABEL)
		{
			//cvRectangle(srcImage,cvPoint(iDrawRectX1,iDrawRectY1),cvPoint(iDrawRectX2,iDrawRectY2),cvScalar(0,0,255),2);
			TLbox.color = RED_PIXEL_LABEL;

			//识别信号灯指向
			int result = RecognizeLight(srcImage, iRect);
			switch (result)
			{
			case 0://圆形
				TLbox.shape = 0;
				break;
			case 1://禁止左转
				TLbox.shape = 1;
				break;
			case 2://前行箭头
				TLbox.shape = 0;
				break;
			default:
				break;
			}
			v.push_back(TLbox);
		}
	}
	else{
		//水平方向统计黑色像素比例
		CvRect HorzRect;
		HorzRect.x = HorzRectX1;
		HorzRect.y = HorzRectY1;
		HorzRect.width = HorzRectWidth;
		HorzRect.height = HorzRectHeight;
		IplImage*HorzLight = cvCreateImage(cvSize(HorzRectWidth, HorzRectHeight), srcImage->depth, srcImage->nChannels);
		GetImageRect(srcImage, HorzRect, HorzLight);
		IplImage *HorzGrayLight = cvCreateImage(cvSize(HorzRectWidth, HorzRectHeight), IPL_DEPTH_8U, 1);
		cvCvtColor(HorzLight, HorzGrayLight, CV_BGR2GRAY);
		cvThreshold(HorzGrayLight, HorzGrayLight, 0, 255, CV_THRESH_OTSU);


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
		/*int cvHorzSum=cvCountNonZero(HorzGrayLight);
		int horzBlackNum=HorzRectWidth*HorzRectHeight-cvHorzSum;*/

		//get the other two  blocks black ration (horizental)
		bool horizBlackLimit = checkOtherBlocksBlackRatio(HorzGrayLight, iColor, false);
		//bool horizBlackLimit = true;

		//else if (HorzBlackRatio>=RatioThreshold&&HorzBlackRatio<=90)
		//else if (horizBlackLimit&&isTL(srcImage, HorzRect, false))
		if (horizBlackLimit&&isTL(srcImage, HorzRect, false))
		{
			//横向检测
			TLbox.box.x = HorzRectX1;
			TLbox.box.y = HorzRectY1;
			TLbox.box.width = HorzRectWidth;
			TLbox.box.height = HorzRectHeight;
			//TLbox.shape = 0;//表示横向



			if (iColor == GREEN_PIXEL_LABEL)
			{
				//cvRectangle(srcImage,cvPoint(HorzRectX1,HorzRectY1),cvPoint(HorzRectX2,HorzRectY2),cvScalar(0,255,0),2);
				TLbox.color = GREEN_PIXEL_LABEL;
				v.push_back(TLbox);
			}

			else if (iColor == RED_PIXEL_LABEL)
			{
				//cvRectangle(srcImage,cvPoint(HorzRectX1,HorzRectY1),cvPoint(HorzRectX2,HorzRectY2),cvScalar(0,0,255),2);
				//*p1=*p1+1;
				TLbox.color = RED_PIXEL_LABEL;
				int result = RecognizeLight(srcImage, iRect);
				switch (result)
				{
				case 0://圆形
					TLbox.shape = 0;
					break;
				case 1://禁止左转
					TLbox.shape = 1;
					break;
				case 2://前行箭头
					TLbox.shape = 0;
					break;
				default:
					break;
				}
				v.push_back(TLbox);
			}
		}
		cvReleaseImage(&HorzLight);
		cvReleaseImage(&HorzGrayLight);
	}





	

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

	//int cvVerticalSum=cvCountNonZero(VerticalGrayLight);
	//int verticalBlackNum=iDrawRectWidth*iDrawRectHeight-cvVerticalSum;//黑色像素点个数
	



	

	//int VerticalBlackRatio = (float)verticalBlackNum*100/(float)((iDrawRectWidth+1)*((float)iDrawRectHeight+1));//矩形框中黑色像素所占比例
	//int HorzBlackRatio=(float)horzBlackNum*100/(float)((HorzRectWidth+1)*((float)HorzRectHeight+1));//矩形框中黑色像素所占比例
	
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

	//int DetectResult=isTL(srcImage,iRect,);
	//int DetectResult = isTL(srcImage, VerticalRect);

	cvReleaseImage(&VerticalLight);
	cvReleaseImage(&VerticalGrayLight);
	

	//DetectResult = 1;

	return;
}