#include "std_tlr.h"
#include "socket_server_task.h"
#include "Drogonfly_ImgRead.h"

Size Win_vertical(15,30),block_vertical(5,10),blockStride_vertical(5,5),cell_vertical(5,5);
HOGDescriptor myHOG_vertical(Win_vertical,block_vertical,blockStride_vertical,cell_vertical,9,1,-1.0,0,0.2,true,64);
HOGDescriptor myHOG_horz(Size(36,12),Size(12,6),Size(6,6),Size(6,6),9,1,-1.0,0,0.2,true,64);
void testSocket();
void testCamera();
vector<Rect> found_filtered;
bool TRAIN=false;
bool HORZ=false;
int Frame_pos;//当前帧位置
CvCapture *capture = NULL;
void On_Change(int n)
{
	cvSetCaptureProperty(capture,CV_CAP_PROP_POS_FRAMES,n);
}


int main()
{
	//socket通信
	SocketInit();
	g_mat = cvCreateMat(2, 1, CV_32FC1);//用于传输数据
	
	CvFont font; 
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, .5, .5, 0, 1, 8);
	if(HORZ)
		hogSVMTrainTL(myHOG_horz,TRAIN,HORZ);
	else
		hogSVMTrainTL(myHOG_vertical,TRAIN,HORZ);
   
	testSocket();
	//testCamera();
	cvReleaseMat(&g_mat);
	system("pause");
    return 0;
}

void testCamera()
{
	//摄像头相关初始化
	CvVideoWriter *writer=NULL;
	bool saveFlag=true;
	Drogonfly_ImgRead p;
	IplImage *frame = NULL,*imageSeg=NULL,*imageNoiseRem =NULL;
	IplImage *resize_tmp=cvCreateImage(Size(800,600),8,3);
	p.Camera_Intial();
	int a[2]={0,0};

	while(1)
	{
		frame=p.Camera2IplImage();
		if(saveFlag)
		{
			writer = cvCreateVideoWriter("testCamera1.avi",CV_FOURCC('X','V','I','D'),10,cvGetSize(resize_tmp),1);
			saveFlag=false;
		}
		if(!frame)break;


		found_filtered.clear();
		//保存视频
		cvWriteFrame(writer,frame); 
		cvWaitKey(10);
		cvResize(frame,resize_tmp);
		imageSeg = colorSegmentation(resize_tmp);
		cvShowImage("imgseg",imageSeg);
		cvWaitKey(5);
		imageNoiseRem=noiseRemoval(imageSeg);
		componentExtraction(imageSeg,resize_tmp,a);
		cvShowImage("resize_frame",resize_tmp);
		cvWaitKey(5);

		//socket通信
		if (!gb_filled)
		{
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 0, 0) = (float)getTickCount();
			if(a[0]>0)//red light
				*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = 9.0;
			if(a[1]>0)//greed light
				*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = 10.0;
			gb_filled = true;
		}
		//如果退出，做相关的清除工作
		int c=cvWaitKey(5);
		if (c==27)
		{
			p.ClearBuffer();
			cvReleaseImage(&resize_tmp);
			cvReleaseVideoWriter(&writer); 
			break;
		}
	}
	cvDestroyAllWindows();
}

void testSocket()
{
	//测试视频相关
	IplImage *frame = NULL,*imageSeg=NULL,*imageNoiseRem =NULL;
	IplImage *resize_tmp=cvCreateImage(Size(800,600),8,3);
	CvVideoWriter *writer=NULL;
	int isColor=1;
	int fps=10;
	int a[2]={0,0};

	capture = cvCreateFileCapture("D:\\JY\\JY_TrainingSamples\\huanhu_clip2.avi");
	int frameFPS=cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);
	int frameNUM=cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_COUNT);
	char Info[200];
	cvNamedWindow("resize_frame");
	cvCreateTrackbar("frame","resize_frame",&Frame_pos,frameNUM,On_Change);

	while (1)
	{
		CvFont font; 
		cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, .5, .5, 0, 1, 8);
		
		int Start=cvGetTickCount();
		frame = cvQueryFrame(capture);
		if(!frame)break;


		found_filtered.clear();
		cvResize(frame,resize_tmp);
		imageSeg = colorSegmentation(resize_tmp);
		cvShowImage("imgseg",imageSeg);
		cvWaitKey(5);
		imageNoiseRem=noiseRemoval(imageSeg);
		componentExtraction(imageSeg,resize_tmp,a);


		//socket通信
		if (!gb_filled)
		{
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 0, 0) = (float)getTickCount();
			if(a[0]>0)//red light
				*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = 9.0;
			if(a[1]>0)//greed light
				*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = 10.0;
			gb_filled = true;
		}

		int currentFrame=cvGetCaptureProperty(capture,CV_CAP_PROP_POS_FRAMES);
		sprintf(Info,"Total frames:%d,current frame:%d",frameNUM,currentFrame);
		//cvRectangle(resize_tmp,Point(0,0),Point(resize_tmp->width,25),Scalar(0,0,0),CV_FILLED);
		cvPutText(resize_tmp,Info,Point(25,17),&font,Scalar(255,255,255));


		cvShowImage("resize_frame",resize_tmp);
		cvWaitKey(5);
		cvWriteFrame(writer,resize_tmp);
		cvReleaseImage(&imageSeg);
		cvReleaseImage(&imageNoiseRem);
		cout << "Frame Grabbed." << endl;
		int End=cvGetTickCount();
		float time=(float)(End-Start)/(cvGetTickFrequency()*1000000);
		cout<<"Time："<<time<<endl;


	}
	cvDestroyAllWindows();
	cvReleaseCapture(&capture);
	cvReleaseImage(&resize_tmp);
	cvReleaseVideoWriter(&writer);
}

void testImages()
{
	int s=0;int i=0; int r=0;int g=0;int a[2]={0,0};
	int fc=0;float  tj=0; 
	//	char c;

	IplImage *frame = NULL,*imageSeg=NULL,*imageNoiseRem =NULL;
	IplImage *tophat,*fix;
	CvSize cv_size;
	cv_size.height=480;
	cv_size.width=640;
	IplImage *resize_tmp=cvCreateImage(cv_size,8,3);
	cvNamedWindow("frame");
	char buff[100];
	//while(frame)
	for(int i=5200;i<6500;i++)
	{

		found_filtered.clear();
		int Start=cvGetTickCount();
		sprintf(buff,"D:\\JY\\JY_TrainingSamples\\Traffic Light Benchmark\\Lara_UrbanSeq1_JPG\\Lara3D_UrbanSeq1_JPG\\%d.jpg",i);
		frame=cvLoadImage(buff);
		cvResize(frame,resize_tmp);
		imageSeg = colorSegmentation(resize_tmp);
		imageNoiseRem = noiseRemoval(imageSeg);
		componentExtraction(imageNoiseRem,resize_tmp,a);
		cvShowImage("frame", resize_tmp);
		cvWaitKey(20);
		cvShowImage("NoiseRem",imageNoiseRem);
		cvWaitKey(20);
		cvReleaseImage(&imageSeg);//由于在函数colorSegmentation中会不停的createImage，因此每一帧都要释放，不然会内存泄露
		cvReleaseImage(&imageNoiseRem);//由于在函数noiseRemoval中会不停的createImage，因此每一帧都要释放，不然会内存泄露
		cvReleaseImage(&frame); //使用cvLoadImage也要每一帧释放，不然内存泄露（应该是cvLoadImage函数也在不停的createImage吧）
		cout << "Frame Grabbed." << endl;
		int End=cvGetTickCount();
		float time=(float)(End-Start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}

	//  cvReleaseCapture(&capture);
	cvReleaseImage(&resize_tmp);
	system("pause");
}