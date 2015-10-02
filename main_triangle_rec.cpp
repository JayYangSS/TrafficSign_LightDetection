#include "HOG_ANN.h"
#include "traffic.h"
#include "math_utils.h"
#include "socket_server_task.h"
#include "Drogonfly_ImgRead.h"
#include "TrafficLightDetection/std_tlr.h"
#include <Windows.h>

//test function
void test_RBYcolor_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void testCamera(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void TLDetection();
void cameraMultiThread();


void covertImg2HOG(Mat img,vector<float> &descriptors)
{
	HOGDescriptor hog(Size(40,40),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,64);
	hog.compute(img,descriptors,Size(8,8));

	cout<<"HOG特征子维数："<<descriptors.size()<<endl;
}

int readdata(String path,int num_folder,String outputfile)
{
	fstream dataSet(outputfile.c_str(),ios::out);
	String img_num,txt_path,folder,img_path;
	stringstream SS_folder;
	Mat img;
	vector<float> pixelVector;
	float ClassId=0;
	int sampleNum=0;
	//folder ID loop
	
	for(int j=1;j<=num_folder;j++)
	{
		ClassId=ClassId+1.0;
		//get the folder name
		SS_folder.clear();//注意清空，不然之前的值不会被覆盖掉
		SS_folder<<j;
		SS_folder>>folder;
		txt_path=path+"\\"+folder+"\\description.txt";
		ifstream txt(txt_path);
		if (!txt)
		{
			cout<<"can't open the txt file!"<<endl;
			exit(1);
		}
		
		while(getline(txt,img_path))
		{
			sampleNum++;
			//read image
			img=imread(img_path);
			Mat resizedImg(IMG_NEW_DIM,IMG_NEW_DIM,CV_8UC3) ;
			resize(img,resizedImg,resizedImg.size());

			covertImg2HOG(resizedImg,pixelVector);
			int img_dim=pixelVector.size();
			for( int l=0 ; l < img_dim; l++)
			{	
				dataSet << pixelVector[l] << " ";
			}

			// save the dataSet in a file.
			dataSet << ClassId << "\n";
		}

	}
	dataSet.close();
	return sampleNum;
}

void shuffleDataSet(string path,string outputfile)
{
	// raw dataset file  8729(rows) * 4800(cols)  not yet shuffle  
	std::ifstream file(path);
	std::string line;

	Mat dataSet;
	int ligne =0;

	// vector of vector containing each line of the dataset file = each image pixels (1*4800)
	vector< vector<double> > vv;


	// iterates through the file to construct the vector vv
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		double n;
		int k = 0;

		vector<double> v;

		while (iss >> n)
		{ 	
			if( k == RESIZED_IMG_DIM +1) break; 
			v.push_back(n);
			k++;
		}

		vv.push_back(v);
		ligne ++ ;

		cout<<"num:"<<ligne<<endl;

	}
	cout<<"put done"<<endl;

	random_shuffle(vv.begin(), vv.end());


	int countPut=0;
	for( int i=0; i < vv.size(); i++)
	{ 
		countPut++;
		double* tab = &vv[i][0];
		Mat img(1,RESIZED_IMG_DIM +1,CV_64FC1,tab);
		dataSet.push_back(img);
		cout<<"countPut:"<<countPut<<endl;
	}
	FileStorage fs(outputfile,FileStorage::WRITE);   
	//fetch the file name(without".yml")
	replace(outputfile.begin(),outputfile.end(),'.',' ');
	stringstream iss(outputfile);
	string outputfileName;
	iss>>outputfileName;
	fs<< outputfileName<< dataSet;
	fs.release(); 
}

void savePCA(string filepath,string outputPath)
{
	Mat dataset;
	// load the shuffled dataSet  ( 8729(rows)  *  48001(cols) )  the last column for the image ClassId	
	FileStorage fs(filepath,FileStorage::READ);

	replace(filepath.begin(),filepath.end(),'.',' ');
	stringstream iss(filepath);
	string readfileName;
	iss>>readfileName;

	fs[readfileName] >> dataset ;
	// exclude the ClassId before performing PCA
	Mat data = dataset(Range::all(), Range(0,RESIZED_IMG_DIM));
	//  perform to retain 99%  of the variance
	PCA pca(data, Mat(), CV_PCA_DATA_AS_ROW , 1.0f);

	// save the model generated for  future uses.
	FileStorage pcaFile(outputPath,FileStorage::WRITE);
	pcaFile << "mean" << pca.mean;
	pcaFile << "e_vectors" << pca.eigenvectors;
	pcaFile << "e_values" << pca.eigenvalues;
	pcaFile.release();
	fs.release();
}

//TL HOG descriptors
Size Win_vertical(15,30),block_vertical(5,10),blockStride_vertical(5,5),cell_vertical(5,5);
HOGDescriptor myHOG_vertical(Win_vertical,block_vertical,blockStride_vertical,cell_vertical,9,1,-1.0,0,0.2,true,64);
HOGDescriptor myHOG_horz(Size(36,12),Size(12,6),Size(6,6),Size(6,6),9,1,-1.0,0,0.2,true,64);
int Frame_pos;//当前帧位置

//control flag
bool isTrain=false;//traffic signs
bool TRAIN=false;//TL
bool HORZ=false;//TL
bool saveFlag=true;
IplImage *resize_TLR=cvCreateImage(Size(800,600),8,3);
IplImage *resize_TSR=cvCreateImage(Size(800,600),8,3);
//lock of thread
HANDLE hMutex;   


DWORD WINAPI TRAFFICLIGHT(LPVOID lpParamter)
{
	TLDetection();
	return 0;
}

DWORD WINAPI TL_FRAME(LPVOID lpParamter)
{
	WaitForSingleObject(hMutex,INFINITE);
	IplImage* frame=(IplImage*)lpParamter;
	IplImage *imageSeg=NULL,*imageNoiseRem =NULL;

	//found_filtered.clear();
	cvResize(frame,resize_TLR);

	//imageSeg = colorSegmentation(resize_TLR);
#if ISDEBUG_TL
	cvShowImage("imgseg",imageSeg);
	cvWaitKey(5);
#endif
	//imageNoiseRem=noiseRemoval(imageSeg);
	//componentExtraction(imageSeg,resize_TLR,a);
	cvShowImage("resize_frame",resize_TLR);
	cvWaitKey(5);
	ReleaseMutex(hMutex);
	return 0;
}
int main()
{
	//socket
	SocketInit();
	g_mat = cvCreateMat(2, 1, CV_32FC1);//用于传输数据
	
	//BP neural network
	CvANN_MLP nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue;
	PCA pca,pca_RoundRim,pca_RoundBlue;
	loadPCA("pcaTriangle.yml", pca);
	loadPCA("pcaRoundRim.yml", pca_RoundRim);
	loadPCA("pcaRoundBlue.yml", pca_RoundBlue);

	//TL detection HOG descriptor
	CvFont font; 
	cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, .5, .5, 0, 1, 8);
	if(HORZ)
		hogSVMTrainTL(myHOG_horz,TRAIN,HORZ);
	else
		hogSVMTrainTL(myHOG_vertical,TRAIN,HORZ);
    
	//BP neural network training
	if(isTrain)
	{
		//神经网络的训练工作
		//triangle
		String path="D:\\JY\\JY_TrainingSamples\\TrafficSign\\triangle";
		int triangleNum=readdata(path,TRIANGLE_CLASSES,"triangle.txt");
		shuffleDataSet("triangle.txt","shuffleTriangle.yml");
		savePCA("shuffleTriangle.yml","pcaTriangle.yml");
		NeuralNetTrain("shuffleTriangle.yml","xmlTriangle.xml",pca,triangleNum,TRIANGLE_CLASSES);
		nnetwork.load("xmlTriangle.xml", "xmlTriangle");

		//RoundRim
		String path_RoundRim="D:\\JY\\JY_TrainingSamples\\TrafficSign\\RoundRim";
		int roundrimNum=readdata(path_RoundRim,ROUNDRIM_CLASSES,"RoundRim.txt");
		shuffleDataSet("RoundRim.txt","shuffleRoundRim.yml");
		savePCA("shuffleRoundRim.yml","pcaRoundRim.yml");
		NeuralNetTrain("shuffleRoundRim.yml","xmlRoundRim.xml",pca_RoundRim,roundrimNum,ROUNDRIM_CLASSES);
		nnetwork_RoundRim.load("xmlRoundRim.xml", "xmlRoundRim");

		//RoundBlue
		String path_RoundBlue="D:\\JY\\JY_TrainingSamples\\TrafficSign\\RoundBlue";
		int roundblueNum=readdata(path_RoundBlue,ROUNDBLUE_CLASSES,"RoundBlue.txt");
		shuffleDataSet("RoundBlue.txt","shuffleRoundBlue.yml");
		savePCA("shuffleRoundBlue.yml","pcaRoundBlue.yml");
		NeuralNetTrain("shuffleRoundBlue.yml","xmlRoundBlue.xml",pca_RoundBlue,roundblueNum,ROUNDBLUE_CLASSES);
		nnetwork_RoundBlue.load("xmlRoundBlue.xml", "xmlRoundBlue");
	}else{
		nnetwork.load("xmlTriangle.xml", "xmlTriangle");
		nnetwork_RoundRim.load("xmlRoundRim.xml", "xmlRoundRim");
		nnetwork_RoundBlue.load("xmlRoundBlue.xml", "xmlRoundBlue");
	}
	
    //create thread to handle TL detection	
	//HANDLE hThread = CreateThread(NULL,0,TRAFFICLIGHT,NULL,0,NULL);
	//	CloseHandle(hThread);

	//test_RBYcolor_Video(pca,pca_RoundRim,pca_RoundBlue,nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue);
	//testCamera(pca,pca_RoundRim,pca_RoundBlue,nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue);
	cameraMultiThread();
	cvReleaseMat(&g_mat);
	system("pause");
}


void TLDetection()
{
	IplImage *frame = NULL,*imageSeg=NULL,*imageNoiseRem =NULL;
	IplImage *resize_tmp=cvCreateImage(Size(800,600),8,3);
	CvCapture *capture=NULL;
	CvVideoWriter *writer=NULL;
	vector<Rect> found_filtered;
	int a[2]={0,0};

	capture = cvCreateFileCapture("D:\\JY\\JY_TrainingSamples\\light2.avi");
	int frameFPS=cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);
	int frameNUM=cvGetCaptureProperty(capture,CV_CAP_PROP_FRAME_COUNT);
	char Info[200];
	cvNamedWindow("resize_frame");

	while (1)
	{
		CvFont font; 
		cvInitFont(&font,CV_FONT_HERSHEY_SIMPLEX, .5, .5, 0, 1, 8);

		int Start=cvGetTickCount();
		frame = cvQueryFrame(capture);
		if(!frame)break;


		found_filtered.clear();
		cvResize(frame,resize_tmp);
		imageSeg = colorSegmentationTL(resize_tmp);

#if ISDEBUG_TL
		cvShowImage("imgseg",imageSeg);
		cvWaitKey(5);
#endif
		imageNoiseRem=noiseRemoval(imageSeg);
		componentExtraction(imageSeg,resize_tmp,a,found_filtered);


		//socket
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
		cvPutText(resize_tmp,Info,Point(25,17),&font,Scalar(255,255,255));
		cvShowImage("resize_frame",resize_tmp);
		cvWaitKey(5);

		//save video
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


void test_RBYcolor_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
										  CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue)
{
	
	float send=0;
	VideoCapture capture; 
	vector<Rect> boundingBox;
	Mat src,re_src,thresh;
	Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};

	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign6.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		
		Mat ihls_image = convert_rgb_to_ihls(re_src);
		//分别对黄蓝红颜色检测
		for (int mode=0;mode<3;mode++)
		{

			Mat nhs_image = convert_ihls_to_nhs(ihls_image,mode);//0:yellow,1:blue,2:red
			Mat noiseremove;
			//分别显示黄蓝红色的nhs二值图像 
			stringstream ss;
			string index;
			ss<<mode;
			ss>>index;
			string tmp="nhs_image"+index;
			//滤波
			medianBlur(nhs_image,noiseremove,3);

#if ISDEBUG_TS
			imshow(tmp,noiseremove);
			waitKey(2);
#endif
			//shape recognition
			Mat p2=ShapeRecognize(noiseremove,boundingBox);
			if(boundingBox.size()==0){
				send=0;
			}
			for (int i=0;i<boundingBox.size();i++)
			{
				Point leftup(boundingBox[i].x,boundingBox[i].y);
				Point rightdown(boundingBox[i].x+boundingBox[i].width,boundingBox[i].y+boundingBox[i].height);
				rectangle(re_src,leftup,rightdown,colorMode[mode],2);
				Mat recognizeMat=re_src(boundingBox[i]);//cut the traffic signs

				//for different color, set different neural network
				if(mode==0)//yellow
				{
					int result=Recognize(nnetwork,pca,recognizeMat,TRIANGLE_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"plus",boundingBox[i]);
						send=1.0;
						break;
					case 2:
						setLabel(re_src,"man",boundingBox[i]);
						send=2.0;break;
					case 3:
						setLabel(re_src,"slow",boundingBox[i]);
						send=3.0;break;
					default:
						break;
					}
				}
				else if(mode==1)//blue
				{
					int result=Recognize(nnetwork_RoundBlue,pca_RoundBlue,recognizeMat,ROUNDBLUE_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"car",boundingBox[i]);
						send=4.0;break;
					case 2:
						setLabel(re_src,"bike",boundingBox[i]);
						send=5.0;break;
					default:
						break;
					}
				}

				else{
					int result=Recognize(nnetwork_RoundRim,pca_RoundRim,recognizeMat,ROUNDRIM_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"NoSound",boundingBox[i]);
						send=6.0;break;
					case 2:
						setLabel(re_src,"30",boundingBox[i]);
						send=7.0;break;
					default:
						break;
					}
				}

			}
			imshow("re_src",re_src);
			waitKey(5);
			boundingBox.clear();//必须清楚当前颜色的框，不然下一种颜色的框的起始位置就不是0了
			
		}
		

		//socket通信
		if (!gb_filled)
		{
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 0, 0) = (float)getTickCount();
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = send;

			gb_filled = true;
		}

		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
}


void testCamera(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue)
{
	//摄像头相关初始化
	IplImage* frame;
	CvVideoWriter *writer=NULL;
	IplImage *resize_tmp=cvCreateImage(Size(800,600),8,3);
	bool saveFlag=true;
	Drogonfly_ImgRead p;
	p.Camera_Intial();

	float send=0;
	vector<Rect> boundingBox;
	Mat src,re_src,thresh;
	Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};

#if SAVEVIDEO
	writer = cvCreateVideoWriter("test6.avi",CV_FOURCC('X','V','I','D'),10,cvGetSize(resize_tmp),1);
#endif
	

	while(1)
	{
		frame=p.Camera2IplImage();
		//保存视频
#if SAVEVIDEO
		cvResize(frame,resize_tmp);
		cvWriteFrame(writer,resize_tmp); 
#endif

		int start=cvGetTickCount();
		src=Mat(frame);
		resize(src,re_src,Size(640,480));
		Mat ihls_image = convert_rgb_to_ihls(re_src);
		//分别对黄蓝红颜色检测
		for (int mode=0;mode<3;mode++)
		{
			Mat nhs_image = convert_ihls_to_nhs(ihls_image,mode);//0:yellow,1:blue,2:red
			Mat noiseremove;
			//分别显示黄蓝红色的nhs二值图像 
			stringstream ss;
			string index;
			ss<<mode;
			ss>>index;
			string tmp="nhs_image"+index;
			//滤波
			medianBlur(nhs_image,noiseremove,3);
			imshow(tmp,noiseremove);
			waitKey(2);
			//形状识别
			Mat p2=ShapeRecognize(noiseremove,boundingBox);
			if(boundingBox.size()==0)
			{
				send=0;
			}
			for (int i=0;i<boundingBox.size();i++)
			{
				Point leftup(boundingBox[i].x,boundingBox[i].y);
				Point rightdown(boundingBox[i].x+boundingBox[i].width,boundingBox[i].y+boundingBox[i].height);
				rectangle(re_src,leftup,rightdown,colorMode[mode],2);
				Mat recognizeMat=re_src(boundingBox[i]);//cut the traffic signs
			
				//for different color, set different neural network
				if(mode==0)//yellow
				{
					int result=Recognize(nnetwork,pca,recognizeMat,TRIANGLE_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"plus",boundingBox[i]);
						send=1.0;
						break;
					case 2:
						setLabel(re_src,"man",boundingBox[i]);
						send=2.0;break;
					case 3:
						setLabel(re_src,"slow",boundingBox[i]);
						send=3.0;break;
					default:
						break;
					}
				}
				else if(mode==1)//blue
				{
					int result=Recognize(nnetwork_RoundBlue,pca_RoundBlue,recognizeMat,ROUNDBLUE_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"car",boundingBox[i]);
						send=4.0;break;
					case 2:
						setLabel(re_src,"bike",boundingBox[i]);
						send=5.0;break;
					default:
						break;
					}
				}
				else{
					int result=Recognize(nnetwork_RoundRim,pca_RoundRim,recognizeMat,ROUNDRIM_CLASSES);
					//set the recognition result to the image
					switch(result)
					{
					case 1:
						setLabel(re_src,"NoSound",boundingBox[i]);
						send=6.0;break;
					case 2:
						setLabel(re_src,"30",boundingBox[i]);
						send=7.0;break;
					default:
						break;
					}
				}
			}
			imshow("re_src",re_src);
			waitKey(5);
			boundingBox.clear();//必须清楚当前颜色的框，不然下一种颜色的框的起始位置就不是0了
		}
		//socket通信
		if (!gb_filled)
		{
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 0, 0) = (float)getTickCount();
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = send;

			gb_filled = true;
		}

		//如果退出，做相关的清除工作
		int c=cvWaitKey(1);
		if (c==27)
		{
			p.ClearBuffer();
			cvReleaseVideoWriter(&writer); 
			break;
		}

		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	


}


void cameraMultiThread()
{
	Drogonfly_ImgRead p;
	p.Camera_Intial();
	IplImage* frame;
	IplImage *resize_tmp=cvCreateImage(Size(800,600),8,3);
	CvVideoWriter *writer=NULL;


#if SAVEVIDEO
	writer = cvCreateVideoWriter("trafficSign7.avi",CV_FOURCC('X','V','I','D'),10,Size(800,600),1);
#endif

	
	hMutex = CreateMutex(NULL, FALSE, "IMG");  
	while(1)
	{
		WaitForSingleObject(hMutex,INFINITE);
		frame=p.Camera2IplImage();
		cvResize(frame,resize_tmp);
		cvShowImage("camera",resize_tmp);
		cvWaitKey(5);

#if SAVEVIDEO
		cvWriteFrame(writer,resize_tmp); 
#endif
		//TLR and TSR
		//create thread to handle TL detection	
		HANDLE hThread = CreateThread(NULL,0,TL_FRAME,frame,0,NULL);
		CloseHandle(hThread);


		//如果退出，做相关的清除工作
		int c=cvWaitKey(1);
		if (c==27)
		{
			p.ClearBuffer();
#if SAVEVIDEO
			cvReleaseVideoWriter(&writer); 
#endif
			break;
		}
		ReleaseMutex(hMutex);  //释放线程锁
	}
}


/*
void testAccuracy(String path,int num_folder)
{
	String img_num,txt_path,folder,img_path;
	stringstream SS_folder;
	Mat img;
	vector<float> pixelVector;
	int sampleNum=0;
	float precision=0;
	//folder ID loop
	int Right=0;
	for(int j=1;j<=num_folder;j++)
	{

		//get the folder name
		SS_folder.clear();//注意清空，不然之前的值不会被覆盖掉
		SS_folder<<j;
		SS_folder>>folder;
		txt_path=path+"\\"+folder+"\\description.txt";
		ifstream txt(txt_path);
		if (!txt)
		{
			cout<<"can't open the txt file!"<<endl;
			exit(1);
		}

		while(getline(txt,img_path))
		{
			sampleNum++;
			cout<<"num="<<sampleNum<<endl;
			//read image
			img=imread(img_path);
			int result=Recognize("xmlTriangle.xml","pcaTriangle.yml",img);
			if (result==j)
				Right++;
		}

	}

	precision=(float)(Right)/(float)(sampleNum);
	
	cout<<"precision="<<precision<<endl;
	
}*/