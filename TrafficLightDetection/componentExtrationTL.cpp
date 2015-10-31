////////////////////////////////////////////////
//2015/10/31:新的提取交通灯的方法
////////////////////////////////////////////////
#include"std_tlr.h"

#define SIZE_FILTER 1
#define REGION_GROW_FILTER 1
#define RECT_FILTER 1

extern deque<float> TLFilters[2];

//识别nhls图像中矩形框内的颜色
int RecColor(Mat img)
{
	int redCount=0;
	int greenCount=0;
	assert(img.channels() == 1);
	for (int i = 0; i < img.rows; ++i)
	{
		const uchar *img_data = img.ptr<uchar> (i);
		for (int j = 0; j < img.cols; ++j)
		{
			uchar pixelVal=*img_data++;
			if(pixelVal==RED_PIXEL_LABEL)
				redCount++;
			else if(pixelVal==GREEN_PIXEL_LABEL)
				greenCount++;
			else
				continue;
		}
	}
	//find max value
	int finalVal=0;
	int tmpCount=0;
	if (redCount>=greenCount)
	{
		tmpCount=redCount;
		finalVal=RED_PIXEL_LABEL;
	}else{
		tmpCount=greenCount;
		finalVal=GREEN_PIXEL_LABEL;
	}
	return finalVal;
}


void componentExtractionTL(IplImage* inputImage,IplImage* srcImage,float* TLDSend)
{	

	int r=0;int g=0;
	const int ministArea=20;
	vector<vector<Point>>contours;
	Rect contourRect;
	Mat inputImg(inputImage);
	Mat edge;
	Canny(inputImg,edge,0,50,5);
	findContours(edge,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	
	for (int i=0;i<contours.size();i++)
	{
		vector<Point> contour=contours[i];
		if (contourArea(contour)<ministArea)continue;
		contourRect=boundingRect(contour);
		int RectWidth=contourRect.width;
		int RectHeight=contourRect.height;

		float WHRatio=(float)RectWidth/(float)RectHeight;
		if (abs(1-WHRatio)>0.2)//constraint of width/height
			continue;

		CvRect iRect;
		iRect.x=contourRect.x;
		iRect.y=contourRect.y;
		iRect.width=contourRect.width;
		iRect.height=contourRect.height;

		//get the color in the contour
		Mat tempMat=inputImg(contourRect);
		int iColor=RecColor(tempMat);
		rectangleDetection(inputImage,srcImage,iRect,iColor,&r,&g);

	}
		
	//filter the result to make it stable
	deque<float>::iterator it;
	int containCount=0;//计算容器中有效检测结果数目
	if (r<1&&g<1)
	{
		for (int i=0;i<2;i++)
		{
			TLFilters[i].push_back(0);
			if (TLFilters[i].size()>5)
				TLFilters[i].pop_front();

			deque<float>::iterator it;
			int TSContainCount=0;
			it=TLFilters[i].begin();
			while(it<TLFilters[i].end())
			{
				if (*it==(float)(i+8))TSContainCount++;
				it++;
			}
#if ISDEBUG_TL
			if (i==0)
				cout<<"TSContainCount:"<<TSContainCount<<endl;
#endif
			if((float)(TSContainCount)/(float)TLFilters[i].size()>=0.4)
				TLDSend[i]=(float)(i+8);
			else
				TLDSend[i]=0;
		}
	}else
	{
		if (r>=1)
		{
			TLFilters[0].push_back(8.0);//red
			if (TLFilters[0].size()>5)
				TLFilters[0].pop_front();

			it=TLFilters[0].begin();
			while (it<TLFilters[0].end())
			{
				if(*it==8.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[0].size()>=0.4)
				TLDSend[0]=8.0;//表示检测到红灯
			else
				TLDSend[0]=0;
			containCount=0;
		}

		if(g>=1)
		{
			TLFilters[1].push_back(9.0);//green
			if (TLFilters[1].size()>5)
				TLFilters[1].pop_front();

			it=TLFilters[1].begin();
			while (it<TLFilters[1].end())
			{
				if(*it==9.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[1].size()>=0.4)
				TLDSend[1]=9.0;//表示检测到绿灯
			else
				TLDSend[1]=0;
			containCount=0;
		}
	}
}



//将与（nSeedX,nSeedY）像素点相同的像素区域找出，区域面积要大于nThreshold*nThreshold时，将区域范围存储入rect中
//若找到则返回true，没找到返回false；被加入聚合区域的像素点相应标志位置（由pUnRegion存储）被置为255
bool regionGrowA(int nSeedX,int nSeedY,BYTE * pUnchInput,int nWidth,int nHeight,
	BYTE * pUnRegion,int nThreshold,int& color,CvRect &rect,int& pixelNum)
{


	int nDx[] = {-1,1,0,0};
	int nDy[] = {0,0,-1,1};
	int nSaveWidth = (nWidth+7)/8*8;//向上补全像素

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


	int seedpointLabel = pUnchInput[nSeedY*nSaveWidth+nSeedX];
	color = seedpointLabel;//单通道图像中种子点像素值



	// 循环控制变量
	int k ;

	// 图象的横纵坐标,用来对当前象素的8邻域进行遍历
	int xx;
	int yy;

	while (nStart<=nEnd)//while这部分的循环作用是把与种子点（nSeedX,nSeedY）像素值相同的像素点坐标全部放入pnGrowQueX[],pnGrowQueY[]中
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
			if ( (xx < nWidth) && (xx>=0) && (yy>=0) && (yy<nHeight) 
				&& (pUnRegion[yy*nWidth+xx]==0) && (pUnchInput[yy*nSaveWidth+xx]==seedpointLabel)) 
			{
				// 堆栈的尾部指针后移一位
				nEnd++;

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
	int nMinx=pnGrowQueX[0], nMaxx=pnGrowQueX[0], nMiny=pnGrowQueY[0], nMaxy = pnGrowQueY[0];//将种子点区域的横纵坐标范围找出
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


	// 释放内存
#if(!TEST)
	delete []pnGrowQueX;
	delete []pnGrowQueY;
	pnGrowQueX = NULL ;
	pnGrowQueY = NULL ;
#endif

	//寻找到边界点，大于一定nThreshold*nThreshold的矩形才可以输出
#if (!TEST)
	if((nMaxy-nMiny)>=nThreshold && (nMaxx - nMinx)>=nThreshold){
		rect.x=nMinx;
		rect.y=nMiny;
		rect.width=nMaxx-nMinx+1;
		rect.height=nMaxy-nMiny+1;
		pixelNum = nEnd;
		return true;
	}
#endif		
	return false;

}