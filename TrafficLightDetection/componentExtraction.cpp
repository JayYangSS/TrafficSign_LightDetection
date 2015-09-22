#include"std_tlr.h"


#define SIZE_FILTER 1
#define REGION_GROW_FILTER 1
#define RECT_FILTER 1

bool regionGrowA(int nSeedX,int nSeedY,BYTE * pUnchInput,int nWidth,int nHeight,
	             BYTE * pUnRegion,int nThreshold,int& color,CvRect &rect,int& pixelNum);


void componentExtraction(IplImage* inputImage,IplImage* srcImage,int* p,vector<Rect> &found_filtered)
{
    int r=0;int g=0;
	int iWidth = inputImage->width;
	int iHeight = inputImage->height;
	int iWidthStep = inputImage->widthStep;
	unsigned char* pImageData = (unsigned char*)inputImage->imageData;//imageData是指向图像数据区域首地址的指针，类型为char*！！！

	IplImage* imageGrayScale = cvCreateImage(cvSize(iWidth,iHeight),IPL_DEPTH_8U,1);
	if(!imageGrayScale)
		exit(EXIT_FAILURE);
	cvCvtColor(srcImage,imageGrayScale,CV_BGR2GRAY);

#if ISDEBUG_TL
	cvShowImage("gray",imageGrayScale);
	cvWaitKey(5);
#endif

	//thresholding for size of components
	const int thresholding = 4;
	int i=0,j=0;
	CvRect oRect;
	//CvRect ooRect;
	int rectNum = 0;
	int rectNum2 = 0;
	int pixelNum=0;
	int oColor=0;
	unsigned char* flag = new unsigned char[iWidth*iHeight];
	if(flag==NULL)
		return;
	memset(flag,0,iWidth*iHeight);//flag矩阵初始化为0
	for(i=0;i<iHeight;i++){
		for(j=0;j<iWidth;j++){

			if(pImageData[i*iWidthStep+j]!=0 && flag[i*iWidth+j]==0){     //图像像素值不为0且没有被处理过
					
				
				//加入聚合区域的像素点相应标志位置（由flag存储）被置为255，种子点的像素值被存储入oColor中，区域内的像素点数目存入pixelNum
				if(regionGrowA(j,i,pImageData,iWidth,iHeight,flag,thresholding,oColor,oRect,pixelNum)){

#if SIZE_FILTER
	
					//printf("%d,%d,%d,%d\n",oRect.x,oRect.y,oRect.width,oRect.height);
					//候选像素区域的外围矩形区域要满足一定的面积，宽高比限制才能被保留
					if(sizeFiltering(oRect)){
						rectNum++;

#if REGION_GROW_FILTER
						//rectangleDetection(imageGrayScale,srcImage,oRect,oColor);
						CvRect ooRect;
						if( regionGrowFiltering(imageGrayScale,srcImage,oRect,ooRect,found_filtered) ){
							rectNum2++;




	///////////////////////////////////////////////////////////////////////////////////////
						///此处要加入判断矩形框内包含交通灯边框的判断函数///
	///////////////////////////////////////////////////////////////////////////////////////


#if  RECT_FILTER
							rectangleDetection(imageGrayScale,srcImage,ooRect,oColor,&r,&g);
							//if(oColor==RED_PIXEL_LABEL)
							//	r=r+1;
							//if(oColor==GREEN_PIXEL_LABEL)
							//	g=g+1;
#endif	//RECT_FILTER

						} //regionGrowFiltering_if
#endif //REGION_GROW_FILTER

					} //sizeFiltering_if
#endif //SIZE_FILTER

				} //regionGrowA_if

			}

		}
	}
	if (r>=1)
		p[0]=1;//p[0]=1，表示检测到红灯
	else p[0]=0;
	if(g>=1)
		p[1]=1;//p[1]=1，表示检测到绿灯
	else p[1]=0;

	if(flag!=NULL){
		delete [] flag;
		flag = NULL;
	}

	cvReleaseImage(&imageGrayScale);


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