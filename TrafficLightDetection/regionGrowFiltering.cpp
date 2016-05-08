#include "std_tlr.h"
#define IS_CUTIMG 0

extern HOGDescriptor myHOG_vertical;
extern HOGDescriptor myHOG_horz;
extern HOGDescriptor TLRecHOG;
extern HOGDescriptor isTLHOG;

extern MySVM TLRecSVM;//识别红色信号灯类别的SVM分类器
//extern MySVM isTLSVM;//识别是否是信号灯
extern MySVM isVerticalTLSVM;//识别识别是否为竖直信号灯的SVM分类器
extern MySVM isHorzTLSVM;//识别识别是否为水平信号灯的SVM分类器
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
int RecognizeLight(IplImage* srcImg,CvRect iRect)
{
	CvSize cutSize;
	cutSize.width=iRect.width;
	cutSize.height=iRect.height;
	IplImage *tmpCutImg=cvCreateImage(cutSize,srcImg->depth,srcImg->nChannels);
	GetImageRect(srcImg,iRect,tmpCutImg);
#if IS_CUTIMG
	cvShowImage("tmpCutImg",tmpCutImg);
	cvWaitKey(1);
	char tmpName[100];
	static int ppp=0;
	ppp++;
	sprintf_s(tmpName,"ImgCut//%d.jpg",ppp);
	cvSaveImage(tmpName,tmpCutImg);
#endif

	Mat cutMat(tmpCutImg);
	Mat tmpTLRec;
	vector<float> descriptor;

	//识别信号灯类别
	resize(cutMat,tmpTLRec,Size(TLREC_WIDTH,TLREC_HEIGHT));
	TLRecHOG.compute(tmpTLRec,descriptor,Size(8,8));
	int DescriptorDim=descriptor.size();		
	Mat SVMTLRecMat(1,DescriptorDim,CV_32FC1);
	for(int i=0; i<DescriptorDim; i++)
		SVMTLRecMat.at<float>(0,i) = descriptor[i];

	int result=TLRecSVM.predict(SVMTLRecMat);
	cvReleaseImage(&tmpCutImg);
	return result;
}



int isTL(IplImage* srcImg,CvRect iRect,bool isVertical)
{
	CvSize cutSize;
	cutSize.width=iRect.width;
	cutSize.height=iRect.height;
	IplImage *tmpCutImg=cvCreateImage(cutSize,srcImg->depth,srcImg->nChannels);
	GetImageRect(srcImg,iRect,tmpCutImg);

	Mat cutMat(tmpCutImg);
	Mat tmpIsTL;
	vector<float> descriptor;

	//识别信号灯类别
	if (isVertical){
		resize(cutMat, tmpIsTL, Size(HOG_TLVertical_Width, HOG_TLVertical_Height));
		myHOG_vertical.compute(tmpIsTL, descriptor, Size(8, 8));
	}
	else{
		resize(cutMat, tmpIsTL, Size(HOG_TLHorz_Width, HOG_TLHorz_Height));
		myHOG_horz.compute(tmpIsTL, descriptor, Size(8, 8));
	}
		
	int DescriptorDim=descriptor.size();		
	Mat SVMTLRecMat(1,DescriptorDim,CV_32FC1);
	for(int i=0; i<DescriptorDim; i++)
		SVMTLRecMat.at<float>(0,i) = descriptor[i];

	//int result=isTLSVM.predict(SVMTLRecMat);
	int result = 0;
	if (isVertical)
		result = isVerticalTLSVM.predict(SVMTLRecMat);
	else
	{
		result = isHorzTLSVM.predict(SVMTLRecMat);
	}
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


