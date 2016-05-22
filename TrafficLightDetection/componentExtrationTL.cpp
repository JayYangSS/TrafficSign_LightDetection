////////////////////////////////////////////////
//2015/10/31:新的提取交通灯的方法
////////////////////////////////////////////////
#include"std_tlr.h"

#define SIZE_FILTER 1
#define REGION_GROW_FILTER 1
#define RECT_FILTER 1
int const nonMatched_green = -1;
int const nonMatched_red = -2;

extern deque<float> TLFilters[3];
extern int TLCount[3];
extern int TLCountThreshold;
extern vector<RectTracker> trackedObj;//当前被跟踪的目标

extern int const containerLen;

bool isCorrelate(Rect &r1, Rect &r2){
	Rect r = r1&r2;
	if (r.width == 0 && r.height == 0)return false;
	else
		return true;
}



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

	vector<ShapeRecResult> currentFrameRect;
	//int p1=0;int p2=0;//p1表示前行位，p2表示左转位,p3表示右转位
	const int ministArea=20;
	vector<vector<Point>>contours;
	Rect contourRect;
	Mat inputImg(inputImage);
	

	Mat edge;
	Canny(inputImg,edge,20,50,5);
	findContours(edge,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);
	/*imshow("edge", edge);
	waitKey(1);*/

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
		rectangleDetection(inputImage, srcImage, iRect, iColor, currentFrameRect);
	}
	//currentFrameRect中的shape：0表示圆形/禁止前行，1表示禁止左转
		if (trackedObj.empty()){
		for (int j = 0; j < currentFrameRect.size(); j++){
			RectTracker rectTracker;
			rectTracker.isDraw = false;
			rectTracker.signs.push_back(1);
			rectTracker.trackedBox = currentFrameRect[j];
			trackedObj.push_back(rectTracker);
		}
	}

	int currentFrameRectNum = currentFrameRect.size();
	int trackedObjNum = trackedObj.size();
	//如果当前没有检测框，所有跟踪目标的滤波容器中push进0
	if (currentFrameRectNum == 0){
		for (int i = 0; i < trackedObjNum; i++){
			trackedObj[i].signs.push_back(0);
			if (trackedObj[i].signs.size()>containerLen)
				trackedObj[i].signs.pop_front();
		}
	}


	
	for (int i = 0; i < trackedObj.size(); i++){
		bool isMatched = false;
		for (int j = 0; j < currentFrameRectNum; j++){
			//跳过currentFrameRect中已经匹配到跟踪目标的元素
			if (currentFrameRect[j].color == nonMatched_green || currentFrameRect[j].color == nonMatched_red)
				continue;

			if (isCorrelate(trackedObj[i].trackedBox.box, currentFrameRect[j].box)){
				trackedObj[i].trackedBox = currentFrameRect[j];
				//将当前帧中的矩形框标记为删除状态
				if (currentFrameRect[j].color==GREEN_PIXEL_LABEL)
					currentFrameRect[j].color = nonMatched_green;
				else if (currentFrameRect[j].color == RED_PIXEL_LABEL)
					currentFrameRect[j].color = nonMatched_red;
				trackedObj[i].signs.push_back(1);
				//如果超过容器长度，弹出最先进入的标志
				if (trackedObj[i].signs.size()>containerLen)trackedObj[i].signs.pop_front();
				isMatched = true;
				break;
			}
		}
		if (isMatched == false){
			trackedObj[i].signs.push_back(0);
			//如果超过容器长度，弹出最先进入的标志
			if (trackedObj[i].signs.size()>containerLen)trackedObj[i].signs.pop_front();
		}

		//超过一定数量未再出现目标，将跟踪目标从trackedObj中删除
		if (trackedObj[i].isCanDelete()){
			trackedObj.erase(trackedObj.begin() + i);
		}
	}

	//遍历currentFrameRect，将其中没有匹配到的矩形框放到trackedObj的最后
	for (int j = 0; j < currentFrameRect.size(); j++){
		if (currentFrameRect[j].color == nonMatched_green || currentFrameRect[j].color == nonMatched_red)
			continue;
		RectTracker rectTracker;
		rectTracker.isDraw = false;
		rectTracker.signs.push_back(1);
		rectTracker.trackedBox = currentFrameRect[j];
		//rectTracker.trackedBox.color = GREEN_PIXEL_LABEL;
		trackedObj.push_back(rectTracker);
			
	}


	//在图像上画出检测框
	for (int i = 0; i < trackedObj.size(); i++){
		if (trackedObj[i].calcDraw()){
			Rect drawRect=trackedObj[i].trackedBox.box;
			int color = trackedObj[i].trackedBox.color;
			CvScalar drawColor = (color == GREEN_PIXEL_LABEL) ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255);
			cvRectangle(srcImage, cvPoint(drawRect.x, drawRect.y), cvPoint(drawRect.x + drawRect.width, drawRect.y + drawRect.height), drawColor, 2);
			switch (trackedObj[i].trackedBox.shape)
			{
			case 0:
				TLDSend[0] = 1;
				break;
			case 1:
				TLDSend[1] = 1;
				break;
			case 2:
				TLDSend[2] = 1;
				break;
			default:
				break;
			}
		}
			
	}


}

