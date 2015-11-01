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
