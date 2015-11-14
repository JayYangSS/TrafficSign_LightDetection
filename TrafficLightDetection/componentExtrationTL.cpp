////////////////////////////////////////////////
//2015/10/31:新的提取交通灯的方法
////////////////////////////////////////////////
#include"std_tlr.h"

#define SIZE_FILTER 1
#define REGION_GROW_FILTER 1
#define RECT_FILTER 1

extern deque<float> TLFilters[3];
extern int TLCount[3];
extern int TLCountThreshold;

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

	int p1=0;int p2=0;//p1表示前行位，p2表示左转位,p3表示右转位
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
		rectangleDetection(inputImage,srcImage,iRect,iColor,&p1,&p2);
	}
		
	//filter the result to make it stable
	deque<float>::iterator it;
	int containCount=0;//计算容器中有效检测结果数目

	//检测结果滤波
/*	if(p1==0&&p2==0)//未检测到信号灯
	{
		for (int i=0;i<3;i++)
		{
			TLFilters[i].push_back(0);
			if (TLFilters[i].size()>5)
				TLFilters[i].pop_front();

			deque<float>::iterator it;
			int TSContainCount=0;
			it=TLFilters[i].begin();
			while(it<TLFilters[i].end())
			{
				if (*it==1.0)TSContainCount++;
				it++;
			}
#if ISDEBUG_TL
			if (i==0)
				cout<<"TSContainCount:"<<TSContainCount<<endl;
#endif
			if((float)(TSContainCount)/(float)TLFilters[i].size()>=0.39)
			{
				TLDSend[i]=1.0;
				TLCount[i]=0;//如果检测到，则计数器清零
			}
				
			else
			{
				//TLDSend[i]=0;
				if(TLCount[i]<TLCountThreshold+5)
				{
					TLCount[i]=TLCount[i]+1;//如果未检测到，则计数器加1
				}
				
				if (TLCount[i]>TLCountThreshold)//如果丢失帧数大于阈值，则认为没有信号灯
				{
					TLDSend[i]=0;
				}else
				{//如果丢失帧数在阈值范围内，则认为还有信号灯
					TLDSend[i]=1.0;
				}
			}
				
		}
	}*/
	
//	else{
		if (p1==1)//前行禁止灯
		{
			TLFilters[0].push_back(1.0);//red
			if (TLFilters[0].size()>5)
				TLFilters[0].pop_front();

			it=TLFilters[0].begin();
			while (it<TLFilters[0].end())
			{
				if(*it==1.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[0].size()>=0.39)
			{
				TLDSend[0]=1.0;//表示检测到红灯
				TLCount[0]=0;//如果检测到，则计数器清零
			}			
			else
			{
				if(TLCount[0]<TLCountThreshold+5)//防止增加的过大溢出
				{
					TLCount[0]=TLCount[0]+1;//如果未检测到，则计数器加1
				}

				if (TLCount[0]>TLCountThreshold)//如果丢失帧数大于阈值，则认为没有信号灯
				{
					TLDSend[0]=0;
				}else
				{//如果丢失帧数在阈值范围内，则认为还有信号灯
					TLDSend[0]=1.0;
				}
			}			
			containCount=0;
		}else{
					TLFilters[0].push_back(0);
					if (TLFilters[0].size()>5)
						TLFilters[0].pop_front();

					deque<float>::iterator it;
					int TSContainCount=0;
					it=TLFilters[0].begin();
					while(it<TLFilters[0].end())
					{
						if (*it==1.0)TSContainCount++;
						it++;
					}

					if((float)(TSContainCount)/(float)TLFilters[0].size()>=0.39)
					{
						TLDSend[0]=1.0;
						TLCount[0]=0;//如果检测到，则计数器清零
					}

					else
					{
						//TLDSend[0]=0;
						if(TLCount[0]<TLCountThreshold+5)
						{
							TLCount[0]=TLCount[0]+1;//如果未检测到，则计数器加1
						}

						if (TLCount[0]>TLCountThreshold)//如果丢失帧数大于阈值，则认为没有信号灯
						{
							TLDSend[0]=0;
						}else
						{//如果丢失帧数在阈值范围内，则认为还有信号灯
							TLDSend[0]=1.0;
						}
					}
		}


		if(p2==1)//左转禁止
		{
			TLFilters[1].push_back(1.0);//green
			if (TLFilters[1].size()>5)
				TLFilters[1].pop_front();

			it=TLFilters[1].begin();
			while (it<TLFilters[1].end())
			{
				if(*it==1.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[1].size()>=0.4)
			{
				TLCount[1]=0;//如果检测到，则计数器清零
				TLDSend[1]=1.0;
			}
				
			else
			{
				if(TLCount[1]<TLCountThreshold+5)//防止增加的过大溢出
				{
					TLCount[1]=TLCount[1]+1;//如果未检测到，则计数器加1
				}

				if (TLCount[1]>TLCountThreshold)//如果丢失帧数大于阈值，则认为没有信号灯
				{
					TLDSend[1]=0;
				}else
				{//如果丢失帧数在阈值范围内，则认为还有信号灯
					TLDSend[1]=1.0;
				}
			}
			containCount=0;
		}else{
			TLFilters[1].push_back(0);
			if (TLFilters[1].size()>5)
				TLFilters[1].pop_front();

			deque<float>::iterator it;
			int TSContainCount=0;
			it=TLFilters[1].begin();
			while(it<TLFilters[1].end())
			{
				if (*it==1.0)TSContainCount++;
				it++;
			}

			if((float)(TSContainCount)/(float)TLFilters[1].size()>=0.39)
			{
				TLDSend[1]=1.0;
				TLCount[1]=0;//如果检测到，则计数器清零
			}

			else
			{
				//TLDSend[0]=0;
				if(TLCount[1]<TLCountThreshold+5)
				{
					TLCount[1]=TLCount[1]+1;//如果未检测到，则计数器加1
				}

				if (TLCount[1]>TLCountThreshold)//如果丢失帧数大于阈值，则认为没有信号灯
				{
					TLDSend[1]=0;
				}else
				{//如果丢失帧数在阈值范围内，则认为还有信号灯
					TLDSend[1]=1.0;
				}
			}
		}

		/*暂时去掉右转检测
		if(p3==1)//右转禁止
		{
			TLFilters[2].push_back(1.0);//green
			if (TLFilters[2].size()>5)
				TLFilters[2].pop_front();

			it=TLFilters[2].begin();
			while (it<TLFilters[2].end())
			{
				if(*it==1.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[2].size()>=0.4)
				TLDSend[2]=1.0;
			else
				TLDSend[2]=0;
			containCount=0;
		}*/
//	}

	//cout<<"count[0]:"<<TLCount[0]<<endl;
	/*	if (r<1&&g<1)//红灯绿灯均无
	{
		for (int i=0;i<3;i++)
		{
			TLFilters[i].push_back(0);
			if (TLFilters[i].size()>5)
				TLFilters[i].pop_front();

			deque<float>::iterator it;
			int TSContainCount=0;
			it=TLFilters[i].begin();
			while(it<TLFilters[i].end())
			{
				if (*it==1.0)TSContainCount++;
				it++;
			}
#if ISDEBUG_TL
			if (i==0)
				cout<<"TSContainCount:"<<TSContainCount<<endl;
#endif
			if((float)(TSContainCount)/(float)TLFilters[i].size()>=0.4)
				TLDSend[i]=1.0;
			else
				TLDSend[i]=0;
		}
	}else
	{
		if (r>=1)
		{
			TLFilters[0].push_back(1.0);//red
			if (TLFilters[0].size()>5)
				TLFilters[0].pop_front();

			it=TLFilters[0].begin();
			while (it<TLFilters[0].end())
			{
				if(*it==1.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[0].size()>=0.4)
				TLDSend[0]=1.0;//表示检测到红灯
			else
				TLDSend[0]=0;
			containCount=0;
		}

		if(g>=1)
		{
			TLFilters[1].push_back(1.0);//green
			if (TLFilters[1].size()>5)
				TLFilters[1].pop_front();

			it=TLFilters[1].begin();
			while (it<TLFilters[1].end())
			{
				if(*it==1.0)containCount++;
				it++;
			}
			if ((float)(containCount)/(float)TLFilters[1].size()>=0.4)
				TLDSend[1]=1.0;//表示检测到绿灯
			else
				TLDSend[1]=0;
			containCount=0;
		}
	}*/
}
