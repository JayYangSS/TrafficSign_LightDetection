#include "std_tlr.h"
#define IS_CUTIMG 0

extern HOGDescriptor myHOG_vertical;
extern HOGDescriptor myHOG_horz;
extern HOGDescriptor TLRecHOG;
extern HOGDescriptor isTLHOG;

extern MySVM TLRecSVM;//识别红色信号灯类别的SVM分类器
extern MySVM isTLSVM;//识别是否是信号灯
extern bool TRAIN;
extern bool HORZ;

#if TEST
extern IplImage* tmpRegion2;
#endif

#if TEST
extern FILE* tmpfp;
#endif

#if 0
bool RegionGrowB(
	   int nSeedX, 
	   int nSeedY, 
	   unsigned char* pUnchInput,
	   int nWidth, 
	   int nHeight, 
	   unsigned char* pUnRegion,
	   int nThreshold,
	   CvRect &rect
	   );
#endif


bool BlackAroundLight(IplImage* srcImg,CvRect	iRect)
{
	bool returnStatus = false;
	int iWidth = srcImg->width,topX=iRect.x;
	int iHeight = srcImg->height,topY=iRect.y;
	int RectWidth,RectHeight;
	if(iRect.width<15)RectWidth=15;
	RectWidth=iRect.width;
	if(iRect.height<30)RectHeight=30;
	RectHeight=iRect.height;
	bool flag=false;



	//设置SVM+HOG处理区域
	CvRect rect;
	if(topX+25>iWidth||topX-20<0)return false;
	else{
		rect.x=iRect.x-20;
		rect.width=45;
	}
	
	if(topY+40>iHeight||topY-30<0)return false;
	else{
		rect.y=iRect.y-30;
		rect.height=70;
	}


	cvSetImageROI(srcImg,rect);
	Mat SVMROI(srcImg);

	cvResetImageROI(srcImg);//这一句少了就出错了啊！！
	if(HORZ)
		flag=BoxDetectTL(SVMROI,myHOG_horz,HORZ);
	flag=BoxDetectTL(SVMROI,myHOG_vertical,HORZ);
	//cvReleaseImage(&srcImg);
	return flag;
}


//not using hole traffic ligh as samples,just use the square light
int RecognizeLight(Mat segImg)
{	
	Mat edge,binImg;
	vector<vector<Point>> contour;
	const int cols = segImg.cols;
	const int rows = segImg.rows;
	const double y_epsTh = 1;
	const double x_epsTh = 1;
	int center_x = cols / 2;
	int center_y = rows / 2;
	double x_eps = 0;
	double y_eps = 0;

	//define the recognition result
	const int circular_go = 0;
	const int direction_left = 1;
	const int direction_right = 2;

	static int count = 0;

	//find contours
	threshold(segImg, binImg, 50, 255, THRESH_BINARY_INV);
	imshow("segImg",segImg);
	waitKey(1);
	imshow("binImg", binImg);
	waitKey(1);

	//save the binary image
	char img_name[50];
	sprintf_s(img_name, "D:\\Img\\%d.jpg", count);
	imwrite(img_name, binImg);
	waitKey(1);
	count++;

	Canny(binImg, edge, 0, 50, 5);
	
	imshow("edge", edge);
	waitKey(1);
	findContours(edge, contour, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	cout << "contour size:"<<contour.size() << endl;
	//use the convexity to determine whether it is the arrow TL
	bool isConvex = isContourConvex(contour[0]);
	if (isConvex){
		cout << "convex" << endl;
		return 0;//return the circular recognition result
	}
	
	//calculate the moments od the TL
	Moments TLMoments = moments(segImg, true);
	int gravityCenter_x = (int)(TLMoments.m10 / TLMoments.m00);
	int gravityCenter_y = (int)(TLMoments.m01 / TLMoments.m00);
	y_eps = gravityCenter_y - center_y;
	x_eps = gravityCenter_x - center_x;

	//return the recogintion result
	if (y_eps > y_epsTh&&abs(x_eps)<x_epsTh){
		return circular_go;
	}
	if (x_eps<(0 - x_epsTh) && abs(y_eps) < y_epsTh){
		return direction_left;//return the left arrow recognition result
	}
	if (x_eps>x_epsTh&&abs(y_eps) < y_epsTh){
		return direction_right;//return the right arrow recognition result
	}
}



int isTL(IplImage* srcImg,CvRect iRect)
{
	CvSize cutSize;
	cutSize.width=iRect.width;
	cutSize.height=iRect.height;
	IplImage *tmpCutImg=cvCreateImage(cutSize,srcImg->depth,srcImg->nChannels);
	GetImageRect(srcImg,iRect,tmpCutImg);
#if IS_CUTIMG
	cvShowImage("tmpCutImg",tmpCutImg);
	cvWaitKey(1);
#endif

	Mat cutMat(tmpCutImg);
	Mat tmpIsTL;
	vector<float> descriptor;

	//识别信号灯类别
	resize(cutMat,tmpIsTL,Size(TLREC_WIDTH,TLREC_HEIGHT));
	isTLHOG.compute(tmpIsTL,descriptor,Size(8,8));
	int DescriptorDim=descriptor.size();		
	Mat SVMTLRecMat(1,DescriptorDim,CV_32FC1);
	for(int i=0; i<DescriptorDim; i++)
		SVMTLRecMat.at<float>(0,i) = descriptor[i];

	int result=isTLSVM.predict(SVMTLRecMat);
	cvReleaseImage(&tmpCutImg);
	return result;
}






bool RegionGrowB2(
				  int nSeedX, 
				  int nSeedY, 
				  unsigned char* pUnchInput,
				  int nWidth, 
				  int nHeight, 
				  unsigned char* pUnRegion,
				  int nThreshold,
				  CvRect &rect
				  );


bool regionGrowFiltering(IplImage* inputImage,IplImage*srcImg,CvRect iRect,CvRect& oRect,vector<Rect>&found_filtered)//找出的矩形框保存到oRect
{
	bool returnStatus = false;
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iWidthStep = inputImage->widthStep;
	unsigned char* pImageData = (unsigned char*)inputImage->imageData;

	//thresholding for graylevel differences between seedpoints and its neibours
	 const int thresholding = 22;//种子点和其邻域像素值之差的阈值

	int i=0,j=0;
	unsigned char* flag = new unsigned char[iWidth*iHeight];
	if(flag==NULL)
		return false;
	memset(flag,0,iWidth*iHeight);

	//矩形框中心点
	int seedX = iRect.x+iRect.width/2;
	int seedY = iRect.y+iRect.height/2;
	
	//pImageData为指向灰度化输入图像的数据区首地址，RegionGrowB2函数可以将像素点数小于maxSizeOfComponents（2000），且灰度值
	//相差小于thresholding（22）的聚集区域找出
	if( RegionGrowB2(seedX,seedY,pImageData,iWidth,iHeight,flag,thresholding,oRect)){
		//if(oRect.width<=iRect.width*2 && oRect.height<=iRect.height*3/2)//此处参数可以调整
		if(oRect.width<=iRect.width*3 && oRect.height<=iRect.height*3&&BlackAroundLight(srcImg,oRect))//此处参数可以调整
			returnStatus = true;
	}

	if(flag!=NULL)
	{
	 delete []flag;
	 flag = NULL;
	}
	
	return returnStatus;
}


#if 0
bool RegionGrowB(
	   int nSeedX, 
	   int nSeedY, 
	   unsigned char* pUnchInput,
	   int nWidth, 
	   int nHeight, 
	   unsigned char* pUnRegion,
	   int nThreshold,
	   CvRect &rect
	   )
{

	const int maxSizeOfComponents = 10000;

	 int nDx[] = {-1,1,0,0};
	 int nDy[] = {0,0,-1,1};
	 int nSaveWidth = (nWidth+7)/8*8;
	  
	 // 定义堆栈，存储坐标
	 int * pnGrowQueX ;
	 int * pnGrowQueY ;

	 // 分配空间
	 pnGrowQueX = new int [nWidth*nHeight];
	 pnGrowQueY = new int [nWidth*nHeight];

	 // 定义堆栈的起点和终点
	 // 当nStart=nEnd, 表示堆栈中只有一个点
	 int nStart ;
	 int nEnd ;

	 //初始化
	 nStart = 0 ;
	 nEnd = 0 ;

	 // 把种子点的坐标压入栈
	 pnGrowQueX[nEnd] = nSeedX;
	 pnGrowQueY[nEnd] = nSeedY;

	 // 当前正在处理的象素
	 int nCurrX ;
	 int nCurrY ;

	 // 循环控制变量
	 int k ;

	 // 图象的横纵坐标,用来对当前象素的8邻域进行遍历
	 int xx;
	 int yy;

	 while (nStart<=nEnd)
	 {
		  // 当前种子点的坐标
		  nCurrX = pnGrowQueX[nStart];
		  nCurrY = pnGrowQueY[nStart];

		  // 对当前点的4邻域进行遍历
		  for (k=0; k<4; k++) 
		  { 
			   // 4邻域象素的坐标
			   xx = nCurrX+nDx[k];
			   yy = nCurrY+nDy[k];

			   // 判断象素(xx，yy) 是否在图像内部
			   // 判断象素(xx，yy) 是否已经处理过
			   // pUnRegion[yy*nWidth+xx]==0 表示还没有处理

			   // 生长条件：判断象素(xx，yy)和当前象素(nCurrX,nCurrY) 象素值差的绝对值
			   if( (xx < nWidth) && (xx>=0) && (yy>=0) && (yy<nHeight) 
				   && (pUnRegion[yy*nWidth+xx]==0) 
				   && abs(pUnchInput[yy*nSaveWidth+xx] - pUnchInput[nCurrY*nSaveWidth+nCurrX])<nThreshold)  
			   {
					// 堆栈的尾部指针后移一位
					nEnd++;

					if(nEnd > maxSizeOfComponents){
						printf("%d\n",nEnd);
						delete []pnGrowQueX;
						delete []pnGrowQueY;
						pnGrowQueX = NULL ;
						pnGrowQueY = NULL ;
						return false;

					}
	
				// 象素(xx，yy) 压入栈
				pnGrowQueX[nEnd] = xx;
				pnGrowQueY[nEnd] = yy;

				// 把象素(xx，yy)设置成逻辑1（255）
				// 同时也表明该象素处理过
				pUnRegion[yy*nWidth+xx] = 255 ;
		   }
		  }
		  nStart++;
	 }
	    
	 
	 //找出区域的范围
	 int nMinx=pnGrowQueX[0], nMaxx=pnGrowQueX[0], nMiny=pnGrowQueY[0], nMaxy = pnGrowQueY[0];
	 for (k=0; k<nEnd; k++)
	 {
	   if (pnGrowQueX[k] > nMaxx)
			 nMaxx = pnGrowQueX[k];
	   if (pnGrowQueX[k] < nMinx) 
			nMinx = pnGrowQueX[k];
	   if (pnGrowQueY[k] > nMaxy)
			nMaxy = pnGrowQueY[k];
	   if (pnGrowQueY[k] < nMiny) 
		   nMiny = pnGrowQueY[k];
	 }

	 rect.x=nMinx;
	 rect.y=nMiny;
	 rect.width=nMaxx-nMinx;
	 rect.height=nMaxy-nMiny;


		// 释放内存
	 delete []pnGrowQueX;
	 delete []pnGrowQueY;
	 pnGrowQueX = NULL ;
	 pnGrowQueY = NULL ;
	 return true;
}
#endif


bool RegionGrowB2(
				 int nSeedX, 
				 int nSeedY, 
				 unsigned char* pUnchInput,
				 int nWidth, 
				 int nHeight, 
				 unsigned char* pUnRegion,
				 int nThreshold,
				 CvRect &rect
				 )
{

	//const int maxSizeOfComponents = 2000;
	const int maxSizeOfComponents = 800;

	int nDx[] = {-1,1,0,0};
	int nDy[] = {0,0,-1,1};
	int nSaveWidth = (nWidth+7)/8*8;

	unsigned int valueSeedSum;
	valueSeedSum = pUnchInput[nSeedY*nSaveWidth+nSeedX];
	//printf("total:%u\n",valueSeedSum);
#if 0  //seedpoint is the average of center
	for(int i=0;i<4;i++)
	{
		//valueSeedX += pUnchInput[nSeedX+nDx[i]];
		//valueSeedY += pUnchInput[nSeedY+nDy[i]];
		valueSeedSum += pUnchInput[(nSeedY+nDy[i])*nSaveWidth+(nSeedX+nDx[i])];
		//printf("total:%u\n",valueSeedSum);
	}
	//valueSeedX /= 5;
	//valueSeedY /= 5;
	unsigned char valueSeed = valueSeedSum/5;
#endif
#if 1  //seedpoint is the center element
	unsigned char valueSeed = valueSeedSum;
#endif
	//printf("pointX:%d,pointY:%d,average:%u\n",nSeedX,nSeedY,valueSeed);

#if TEST
	fprintf(tmpfp,"pointX:%d,pointY:%d,average:%u\n",nSeedX,nSeedY,valueSeed);
#endif

	// 定义堆栈，存储坐标
	int * pnGrowQueX ;
	int * pnGrowQueY ;

	// 分配空间
	pnGrowQueX = new int [nWidth*nHeight];
	pnGrowQueY = new int [nWidth*nHeight];

	// 定义堆栈的起点和终点
	// 当nStart=nEnd, 表示堆栈中只有一个点
	int nStart ;
	int nEnd ;

	//初始化
	nStart = 0 ;
	nEnd = 0 ;

	// 把种子点的坐标压入栈
	pnGrowQueX[nEnd] = nSeedX;
	pnGrowQueY[nEnd] = nSeedY;

	// 当前正在处理的象素
	int nCurrX ;
	int nCurrY ;

	// 循环控制变量
	int k ;

	// 图象的横纵坐标,用来对当前象素的8邻域进行遍历
	int xx;
	int yy;

	while (nStart<=nEnd)
	{
		// 当前种子点的坐标
		nCurrX = pnGrowQueX[nStart];
		nCurrY = pnGrowQueY[nStart];

		// 对当前点的4邻域进行遍历
		for (k=0; k<4; k++) 
		{ 
			// 4邻域象素的坐标
			xx = nCurrX+nDx[k];
			yy = nCurrY+nDy[k];

			// 判断象素(xx，yy) 是否在图像内部
			// 判断象素(xx，yy) 是否已经处理过
			// pUnRegion[yy*nWidth+xx]==0 表示还没有处理

			// 生长条件：判断象素(xx，yy)和当前象素(nCurrX,nCurrY) 象素值差的绝对值
			if( (xx < nWidth) && (xx>=0) && (yy>=0) && (yy<nHeight) 
				&& (pUnRegion[yy*nWidth+xx]==0) 
				&& abs( pUnchInput[yy*nSaveWidth+xx] - valueSeed )<nThreshold)  
			{
				// 堆栈的尾部指针后移一位
				nEnd++;

				if(nEnd > maxSizeOfComponents){
					//printf("%d\n",nEnd);
					delete []pnGrowQueX;
					delete []pnGrowQueY;
					pnGrowQueX = NULL ;
					pnGrowQueY = NULL ;
					return false;

				}

				// 象素(xx，yy) 压入栈
				pnGrowQueX[nEnd] = xx;
				pnGrowQueY[nEnd] = yy;

				// 把象素(xx，yy)设置成逻辑1（255）
				// 同时也表明该象素处理过
				pUnRegion[yy*nWidth+xx] = 255 ;
			}
		}
		nStart++;
	}


	//找出区域的范围
	int nMinx=pnGrowQueX[0], nMaxx=pnGrowQueX[0], nMiny=pnGrowQueY[0], nMaxy = pnGrowQueY[0];
	for (k=0; k<nEnd; k++)
	{
		if (pnGrowQueX[k] > nMaxx)
			nMaxx = pnGrowQueX[k];
		if (pnGrowQueX[k] < nMinx) 
			nMinx = pnGrowQueX[k];
		if (pnGrowQueY[k] > nMaxy)
			nMaxy = pnGrowQueY[k];
		if (pnGrowQueY[k] < nMiny) 
			nMiny = pnGrowQueY[k];
	}

	rect.x=nMinx;
	rect.y=nMiny;
	rect.width=nMaxx-nMinx+1;
	rect.height=nMaxy-nMiny+1;

#if TEST
	for(int k=0;k<nEnd;k++)
	{
		tmpRegion2->imageData[(pnGrowQueY[k]*nSaveWidth+pnGrowQueX[k])] = 255;
	}
#endif


	// 释放内存
	delete []pnGrowQueX;
	delete []pnGrowQueY;
	pnGrowQueX = NULL ;
	pnGrowQueY = NULL ;
	return true;
}


