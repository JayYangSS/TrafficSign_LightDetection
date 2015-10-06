#include "HOG_ANN.h"
#include "traffic.h"
#include "math_utils.h"
#include "socket_server_task.h"
#include "Drogonfly_ImgRead.h"
#include "TrafficLightDetection/std_tlr.h"
#include <Windows.h>
#include <queue>


//TL HOG descriptors
Size Win_vertical(15,30),block_vertical(5,10),blockStride_vertical(5,5),cell_vertical(5,5);
HOGDescriptor myHOG_vertical(Win_vertical,block_vertical,blockStride_vertical,cell_vertical,9,1,-1.0,0,0.2,true,64);
HOGDescriptor myHOG_horz(Size(36,12),Size(12,6),Size(6,6),Size(6,6),9,1,-1.0,0,0.2,true,64);
int Frame_pos;//当前帧位置

//control TSR_flag
bool isTrain=false;//traffic signs
bool TRAIN=false;//TL
bool HORZ=false;//TL
bool saveFlag=true;
IplImage *resize_TLR=cvCreateImage(Size(800,600),8,3);

vector<Rect> found_TL;//the bounding box for traffic lights
vector<Rect> found_TSR;//the bounding box for traffic signs
Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};//the color mode for the traffic sign detection(Y,B,R)
CvANN_MLP nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue;//neural networks for three different kinds of traffic signs 
PCA pca,pca_RoundRim,pca_RoundBlue;
deque<float> signFilters[7];
deque<float> TLFilters[2];
bool TSR_flag[7]={false,false,false,false,false,false,false};//Traffic sign control flag
bool TLD_flag[2]={false,false};//traffic lighs control flags


//test function
void test_RBYcolor_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void testCamera(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void TLDetection();
void cameraMultiThread();
void videoMultiThread();
void test_RBYcolorMerge_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void openMP_MultiThreadVideo();


Point2d getBoxCenter(Rect &boundingBox){
	Point2d centerPoint;
	centerPoint.x=boundingBox.x+boundingBox.width/2;
	centerPoint.y=boundingBox.y+boundingBox.height/2;
	return centerPoint;
}

void getCentersFromBoxes(vector<Rect> &boundingBoxs,vector<Point2d> &centers)
{
	for(int i=0;i<boundingBoxs.size();i++)
	{
		Point2d center=getBoxCenter(boundingBoxs[i]);
		centers.push_back(center);
	}
}

Rect getSearchRegion(Point2d center,Size windowSize,Size imageSize)
{
	Rect searchRegion;
	int imgWidth=imageSize.width;
	int imgHeight=imageSize.height;
	int windowWidth=windowSize.width;
	int windowHeight=windowSize.height;
	//横坐标
	if(center.x-windowWidth/2<0)
	{
		searchRegion.x=0;
		searchRegion.width=windowWidth/2+(int)(center.x);
	}
	else if(center.x+windowWidth/2>imgWidth)
	{
		searchRegion.x=center.x-windowWidth/2;
		searchRegion.width=windowWidth/2+imgWidth-center.x;
	}
	else
	{
		searchRegion.x=center.x-windowWidth/2;
		searchRegion.width=windowWidth;
	}
	//纵坐标
	if(center.y-windowHeight/2<0)
	{
		searchRegion.y=0;
		searchRegion.height=windowHeight/2+(int)(center.y);
	}
	else if(center.y+windowHeight/2>imgHeight)
	{
		searchRegion.y=center.y-windowHeight/2;
		searchRegion.height=windowHeight/2+imgHeight-center.y;
	}
	else
	{
		searchRegion.y=center.y-windowHeight/2;
		searchRegion.height=windowHeight;
	}
	return searchRegion;
}

bool isContainSigns(Mat img,Rect searchRegion,float thresholdRatio)
{
	Mat searchMat=img(searchRegion);//cut the traffic signs
	int count=0;
	//visit every pixel
	int nRows=searchMat.rows;
	int nCols=searchMat.cols;
	if (searchMat.isContinuous())
	{
		nCols *= nRows;  
		nRows = 1;
	}

	for(int i=0;i<nRows;i++)
	{
		uchar* p=searchMat.ptr<uchar>(i);
		for(int j=0;j<nCols;j++)
		{
			if(p[j]==255)
				count++;
		}
	}
	if(count>thresholdRatio*nRows*nCols)
		return true;
	return false;
}

void covertImg2HOG(Mat img,vector<float> &descriptors)
{
	HOGDescriptor hog(Size(40,40),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,64);
	hog.compute(img,descriptors,Size(8,8));

	cout<<"HOG特征子维数："<<descriptors.size()<<endl;
}

//get the HOG features(float array) of each image in the specified folder
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


void TLDetectionPerFrame(IplImage *frame,float *TLDSend)
{
	IplImage *imageSeg=NULL,*imageNoiseRem =NULL;

	found_TL.clear();
	cvResize(frame,resize_TLR);

	imageSeg = colorSegmentationTL(resize_TLR);
#if ISDEBUG_TL
	cvNamedWindow("imgseg");
	cvShowImage("imgseg",imageSeg);
	cvWaitKey(5);
#endif
	imageNoiseRem=noiseRemoval(imageSeg);
	componentExtraction(imageSeg,resize_TLR,TLDSend,found_TL);
	cvNamedWindow("TL");
	cvShowImage("TL",resize_TLR);
	cvWaitKey(5);

	cvReleaseImage(&imageSeg);
	cvReleaseImage(&imageNoiseRem);
	cvReleaseImage(&frame);
}

void TSRecognitionPerFrame(IplImage *frame,float *TSRSend)
{
	vector<ShapeRecResult> shapeResult;
	Mat src(frame);
	Mat re_src;
	resize(src,re_src,Size(640,480));
	Mat bilateralImg;
	bilateralFilter(re_src,bilateralImg,7,7*2,7/2);
	int count=0;
#if ISDEBUG_TS
	namedWindow("bilateralImg");
	imshow("bilateralImg",bilateralImg);
	waitKey(5);
#endif
	Mat ihls_image = convert_rgb_to_ihls(bilateralImg);
	Mat nhs_image=convert_ihls_to_seg(ihls_image);

#if ISDEBUG_TS
	namedWindow("seg");
	imshow("seg",nhs_image);
	waitKey(5);
#endif

	Mat noiseremove;
	int erosion_size=1;
	int eroionType=MORPH_CROSS;
	Mat element = getStructuringElement( eroionType,Size( 2*erosion_size + 1, 2*erosion_size+1 ),Point( erosion_size, erosion_size ) );
	erode( nhs_image, noiseremove, element);
#if ISDEBUG_TS
	namedWindow("morph");
	imshow("morph",noiseremove);
	waitKey(2);
#endif
	Mat labeledImg=ShapeRecognize(noiseremove,shapeResult);

#if ISDEBUG_TS
	namedWindow("labeledImg");
	imshow("labeledImg",labeledImg);
	waitKey(5);
#endif
	for (int i=0;i<shapeResult.size();i++)
	{
		Rect boundingBox=shapeResult[i].box;
		Point leftup(boundingBox.x,boundingBox.y);
		Point rightdown(boundingBox.x+boundingBox.width,boundingBox.y+boundingBox.height);
	//	rectangle(re_src,leftup,rightdown,colorMode[2],2);
		Mat recognizeMat=re_src(boundingBox);//cut the traffic signs
		//int count=0;
		deque<float>::iterator it;

		//for different color, set different neural network
		if(shapeResult[i].shape==TRIANGLE&&shapeResult[i].color==Y_VALUE)//yellow
		{
			rectangle(re_src,leftup,rightdown,colorMode[0],2);
			int result=Recognize(nnetwork,pca,recognizeMat,TRIANGLE_CLASSES);
			//set the recognition result to the image
			switch(result)
			{
			case 1:
				setLabel(re_src,"plus",boundingBox);
				//TSRSend[0]=1.0;break;
				signFilters[0].push_back(1.0);
				if (signFilters[0].size()>5)
					signFilters[0].pop_front();
				TSR_flag[0]=true;
				it=signFilters[0].begin();
				while (it<signFilters[0].end())
				{
					if(*it==1.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[0].size()>=0.4)
				{
					TSRSend[0]=1.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[0]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	

			case 2:
				setLabel(re_src,"man",boundingBox);
				//TSRSend[1]=2.0;break;
				signFilters[1].push_back(2.0);
				if (signFilters[1].size()>5)
					signFilters[1].pop_front();
				TSR_flag[1]=true;
				it=signFilters[1].begin();
				while (it<signFilters[1].end())
				{
					if(*it==2.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[1].size()>=0.4)
				{
					TSRSend[1]=2.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[1]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	

			case 3:
				setLabel(re_src,"slow",boundingBox);
				//TSRSend[2]=3.0;break;
				signFilters[2].push_back(3.0);
				if (signFilters[2].size()>5)
					signFilters[2].pop_front();
				TSR_flag[2]=true;
				it=signFilters[2].begin();
				while (it<signFilters[2].end())
				{
					if(*it==3.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[2].size()>=0.4)
				{
					TSRSend[2]=3.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[2]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	
			case 4:
				setLabel(re_src,"work",boundingBox);
				break;
			default:
				break;
			}
		}
		else if(shapeResult[i].shape==CIRCLE&&shapeResult[i].color==B_VALUE)//circle
		{
			rectangle(re_src,leftup,rightdown,colorMode[1],2);
			int result=Recognize(nnetwork_RoundBlue,pca_RoundBlue,recognizeMat,ROUNDBLUE_CLASSES);
			//set the recognition result to the image
			switch(result)
			{
			case 1:
				setLabel(re_src,"car",boundingBox);
				//TSRSend[3]=4.0;break;
				signFilters[3].push_back(4.0);
				if (signFilters[3].size()>5)
					signFilters[3].pop_front();
				TSR_flag[3]=true;
				it=signFilters[3].begin();
				while (it<signFilters[3].end())
				{
					if(*it==4.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[3].size()>=0.4)
				{
					TSRSend[3]=4.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[3]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	

			case 2:
				setLabel(re_src,"bike",boundingBox);
				//TSRSend[4]=5.0;break;
				signFilters[4].push_back(5.0);
				if (signFilters[4].size()>5)
					signFilters[4].pop_front();
				TSR_flag[4]=true;
				it=signFilters[4].begin();
				while (it<signFilters[4].end())
				{
					if(*it==5.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[4].size()>=0.4)
				{
					TSRSend[4]=5.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[4]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	

			default:
				break;
			}
		}

		else{
			rectangle(re_src,leftup,rightdown,colorMode[2],2);
			int result=Recognize(nnetwork_RoundRim,pca_RoundRim,recognizeMat,ROUNDRIM_CLASSES);
			//set the recognition result to the image

			switch(result)
			{
			case 1:
				setLabel(re_src,"NoSound",boundingBox);
				//TSRSend[5]=6.0;break;
				signFilters[5].push_back(6.0);
				if (signFilters[5].size()>5)
					signFilters[5].pop_front();
				TSR_flag[5]=true;
				it=signFilters[5].begin();
				while (it<signFilters[5].end())
				{
					if(*it==6.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[5].size()>=0.4)
				{
					TSRSend[5]=6.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[5]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	

			case 2:
				setLabel(re_src,"30",boundingBox);
				signFilters[6].push_back(7.0);
				if (signFilters[6].size()>5)
					signFilters[6].pop_front();
				TSR_flag[6]=true;
				it=signFilters[6].begin();
				while (it<signFilters[6].end())
				{
					if(*it==7.0)count++;
					it++;
				}
				if((float)(count)/(float)signFilters[6].size()>=0.4)
				{
					TSRSend[6]=7.0;
					//cout<<"detected"<<endl;
				}
				else
				{
					TSRSend[6]=0.0;
					//cout<<"No detected"<<endl;
				}
				count=0;
				break;	
				//TSRSend=7.0;break;
			case 3:
				setLabel(re_src,"stop",boundingBox);
				break;
			default:
				break;
			}
		}

	}

	for (int i=0;i<=6;i++)
	{
		if(!TSR_flag[i])
		{
			signFilters[i].push_back(0);
			if (signFilters[i].size()>5)
				signFilters[i].pop_front();
		}
		//cout<<TSRSend[i]<<" ";


		deque<float>::iterator it;
		int containCount=0;//计算容器中有效检测结果数目
		it=signFilters[i].begin();
		while (it<signFilters[i].end())
		{
			if((*it)==(float)(i+1))
				containCount++;
			it++;
		}
		if((float)(containCount)/(float)signFilters[i].size()>=0.4)
		{
			TSRSend[i]=(float)(i+1);
			//cout<<"detected"<<endl;
		}
		else
		{
			TSRSend[i]=0.0;
			//cout<<"No detected"<<endl;
		}

	}

	shapeResult.clear();
	namedWindow("TSR");
	imshow("TSR",re_src);
	waitKey(5);
}

int main()
{
	//socket
	SocketInit();
	g_mat = cvCreateMat(2, 1, CV_32FC1);//transmit data

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
		//triangle
		String path="D:\\JY\\JY_TrainingSamples\\TrafficSign\\triangle";
		int triangleNum=readdata(path,TRIANGLE_CLASSES,"triangle.txt");
		shuffleDataSet("triangle.txt","shuffleTriangle.yml");
		savePCA("shuffleTriangle.yml","pcaTriangle.yml");
		loadPCA("pcaTriangle.yml", pca);
		NeuralNetTrain("shuffleTriangle.yml","xmlTriangle.xml",pca,triangleNum,TRIANGLE_CLASSES);
		nnetwork.load("xmlTriangle.xml", "xmlTriangle");

		//RoundRim
		String path_RoundRim="D:\\JY\\JY_TrainingSamples\\TrafficSign\\RoundRim";
		int roundrimNum=readdata(path_RoundRim,ROUNDRIM_CLASSES,"RoundRim.txt");
		shuffleDataSet("RoundRim.txt","shuffleRoundRim.yml");
		savePCA("shuffleRoundRim.yml","pcaRoundRim.yml");
		loadPCA("pcaRoundRim.yml", pca_RoundRim);
		NeuralNetTrain("shuffleRoundRim.yml","xmlRoundRim.xml",pca_RoundRim,roundrimNum,ROUNDRIM_CLASSES);
		nnetwork_RoundRim.load("xmlRoundRim.xml", "xmlRoundRim");

		//RoundBlue
		String path_RoundBlue="D:\\JY\\JY_TrainingSamples\\TrafficSign\\RoundBlue";
		int roundblueNum=readdata(path_RoundBlue,ROUNDBLUE_CLASSES,"RoundBlue.txt");
		shuffleDataSet("RoundBlue.txt","shuffleRoundBlue.yml");
		savePCA("shuffleRoundBlue.yml","pcaRoundBlue.yml");
		loadPCA("pcaRoundBlue.yml", pca_RoundBlue);
		NeuralNetTrain("shuffleRoundBlue.yml","xmlRoundBlue.xml",pca_RoundBlue,roundblueNum,ROUNDBLUE_CLASSES);
		nnetwork_RoundBlue.load("xmlRoundBlue.xml", "xmlRoundBlue");
	}else{
		loadPCA("pcaTriangle.yml", pca);
		loadPCA("pcaRoundRim.yml", pca_RoundRim);
		loadPCA("pcaRoundBlue.yml", pca_RoundBlue);
		nnetwork.load("xmlTriangle.xml", "xmlTriangle");
		nnetwork_RoundRim.load("xmlRoundRim.xml", "xmlRoundRim");
		nnetwork_RoundBlue.load("xmlRoundBlue.xml", "xmlRoundBlue");
	}
	
	//test_RBYcolor_Video(pca,pca_RoundRim,pca_RoundBlue,nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue);
	//testCamera(pca,pca_RoundRim,pca_RoundBlue,nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue);
	//cameraMultiThread();
	//videoMultiThread();
	//TLDetection();
	openMP_MultiThreadVideo();
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
	float TLDSend[2]={0,0};

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
		componentExtraction(imageSeg,resize_tmp,TLDSend,found_filtered);


		//socket
		if (!gb_filled)
		{
			*(float *)CV_MAT_ELEM_PTR(*g_mat, 0, 0) = (float)getTickCount();
			if(TLDSend[0]>0)//red light
				*(float *)CV_MAT_ELEM_PTR(*g_mat, 1, 0) = 9.0;
			if(TLDSend[1]>0)//greed light
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



void openMP_MultiThreadVideo()
{
	CvCapture * cap=cvCreateFileCapture("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign6.avi");
	//CvCapture * cap=cvCreateFileCapture("D:\\JY\\JY_TrainingSamples\\light2.avi");
	IplImage * frame,*copyFrame;
	float connectResult[9]={0,0,0,0,0,0,0,0,0};
	while(1)
	{
		float TSRSend[7]={0,0,0,0,0,0,0};//store the traffic signs recognition result
		float TLDSend[2]={0,0};//store the traffic lights detection result

		int start=cvGetTickCount();
		frame=cvQueryFrame(cap);
		if(!frame)break;
		//MultiThread
		cvNamedWindow("TL");
		namedWindow("TSR");
#if ISDEBUG_TL
		cvNamedWindow("imgseg");
#endif
		//copyFrame=cvCloneImage(frame);
		copyFrame=cvCreateImage(Size(frame->width,frame->height),frame->depth,frame->nChannels);
		cvCopy(frame,copyFrame);

#pragma omp parallel sections
		{
#pragma omp section
			{
				//TSR 
				TSRecognitionPerFrame(frame,TSRSend);
			}

#pragma omp section
			{
				//TL detection
				TLDetectionPerFrame(copyFrame,TLDSend);
			}
		}

		//get the union result
		for (int i=0;i<7;i++)
		{
			connectResult[i]=TSRSend[i];
		}
		for (int i=0;i<2;i++)
		{
			connectResult[7+i]=TLDSend[i];
		}

		for (int i=0;i<9;i++)
		{
			cout<<connectResult[i]<<" ";
		}
	//	cout<<" "<<endl;
		char c=waitKey(5);
		if (c==27)break;

		int end=cvGetTickCount();
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}
	cvReleaseCapture(&cap);
	cvDestroyAllWindows();
}


void test_RBYcolorMerge_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
										  CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue)
{
	VideoCapture capture; 
	vector<Rect> boundingBox;
	vector<ShapeRecResult> shapeResult;
	Mat src,re_src,thresh;
	Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};
	//kalman tracker
	vector<Point2d> centers;
	vector<Point2d> storePredictCenters;
	Rect searchRegion;
	//滤波容器
	deque<float> signFilters[7];
	
	//process every frame
	capture.open("D:\\JY\\JY_TrainingSamples\\比赛视频截取\\StopSign.mp4");
	while(capture.read(src))
	{
		float TSRSend[7]={0,0,0,0,0,0,0};
		bool TSR_flag[7]={false,false,false,false,false,false,false};
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		Mat re_src1;
		//GaussianBlur(re_src,re_src1,Size(5,5),0,0);
		bilateralFilter(re_src,re_src1,7,7*2,7/2);
		imshow("gaussion",re_src1);
		waitKey(5);
		Mat ihls_image = convert_rgb_to_ihls(re_src1);
		Mat nhs_image=convert_ihls_to_seg(ihls_image);
		imshow("seg",nhs_image);
		waitKey(5);
		Mat noiseremove;
		int erosion_size=1;
		int eroionType=MORPH_CROSS;
		Mat element = getStructuringElement( eroionType,Size( 2*erosion_size + 1, 2*erosion_size+1 ),Point( erosion_size, erosion_size ) );
		erode( nhs_image, noiseremove, element);
		imshow("morph",noiseremove);
		waitKey(2);
	

		Mat labeledImg=ShapeRecognize(noiseremove,shapeResult);
		imshow("labeledImg",labeledImg);
		waitKey(5);
		for (int i=0;i<shapeResult.size();i++)
		{
			Rect boundingBox=shapeResult[i].box;
			Point leftup(boundingBox.x,boundingBox.y);
			Point rightdown(boundingBox.x+boundingBox.width,boundingBox.y+boundingBox.height);
			rectangle(re_src,leftup,rightdown,colorMode[2],2);
			Mat recognizeMat=re_src(boundingBox);//cut the traffic signs
			int count=0;
			deque<float>::iterator it;

			//for different color, set different neural network
			if(shapeResult[i].shape==TRIANGLE&&shapeResult[i].color==Y_VALUE)//yellow
			{
				int result=Recognize(nnetwork,pca,recognizeMat,TRIANGLE_CLASSES);
				//set the recognition result to the image
				switch(result)
				{
				case 1:
					setLabel(re_src,"plus",boundingBox);
					//TSRSend[0]=1.0;break;
					signFilters[0].push_back(1.0);
					if (signFilters[0].size()>5)
						signFilters[0].pop_front();
					TSR_flag[0]=true;
					it=signFilters[0].begin();
					while (it<signFilters[0].end())
					{
						if(*it==1.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[0].size()>=0.4)
					{
						TSRSend[0]=1.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[0]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"man",boundingBox);
					//TSRSend[1]=2.0;break;
					signFilters[1].push_back(2.0);
					if (signFilters[1].size()>5)
						signFilters[1].pop_front();
					TSR_flag[1]=true;
					it=signFilters[1].begin();
					while (it<signFilters[1].end())
					{
						if(*it==2.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[1].size()>=0.4)
					{
						TSRSend[1]=2.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[1]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 3:
					setLabel(re_src,"slow",boundingBox);
					//TSRSend[2]=3.0;break;
					signFilters[2].push_back(3.0);
					if (signFilters[2].size()>5)
						signFilters[2].pop_front();
					TSR_flag[2]=true;
					it=signFilters[2].begin();
					while (it<signFilters[2].end())
					{
						if(*it==3.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[2].size()>=0.4)
					{
						TSRSend[2]=3.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[2]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	
				case 4:
					setLabel(re_src,"work",boundingBox);
					break;
				default:
					break;
				}
			}
			else if(shapeResult[i].shape==CIRCLE&&shapeResult[i].color==B_VALUE)//circle
			{
				int result=Recognize(nnetwork_RoundBlue,pca_RoundBlue,recognizeMat,ROUNDBLUE_CLASSES);
				//set the recognition result to the image
				switch(result)
				{
				case 1:
					setLabel(re_src,"car",boundingBox);
					//TSRSend[3]=4.0;break;
					signFilters[3].push_back(4.0);
					if (signFilters[3].size()>5)
						signFilters[3].pop_front();
					TSR_flag[3]=true;
					it=signFilters[3].begin();
					while (it<signFilters[3].end())
					{
						if(*it==4.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[3].size()>=0.4)
					{
						TSRSend[3]=4.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[3]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"bike",boundingBox);
					//TSRSend[4]=5.0;break;
					signFilters[4].push_back(5.0);
					if (signFilters[4].size()>5)
						signFilters[4].pop_front();
					TSR_flag[4]=true;
					it=signFilters[4].begin();
					while (it<signFilters[4].end())
					{
						if(*it==5.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[4].size()>=0.4)
					{
						TSRSend[4]=5.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[4]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

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
					setLabel(re_src,"NoSound",boundingBox);
					//TSRSend[5]=6.0;break;
					signFilters[5].push_back(6.0);
					if (signFilters[5].size()>5)
						signFilters[5].pop_front();
					TSR_flag[5]=true;
					it=signFilters[5].begin();
					while (it<signFilters[5].end())
					{
						if(*it==6.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[5].size()>=0.4)
					{
						TSRSend[5]=6.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[5]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"30",boundingBox);
					signFilters[6].push_back(7.0);
					if (signFilters[6].size()>5)
						signFilters[6].pop_front();
					TSR_flag[6]=true;
					it=signFilters[6].begin();
					while (it<signFilters[6].end())
					{
						if(*it==7.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[6].size()>=0.4)
					{
						TSRSend[6]=7.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						TSRSend[6]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	
					//TSRSend=7.0;break;
				case 3:
					setLabel(re_src,"stop",boundingBox);
					break;
				default:
					break;
				}
			}
			
		}

		for (int i=0;i<=6;i++)
		{
			if(!TSR_flag)
			{
				signFilters[i].push_back(0);
				if (signFilters[i].size()>5)
					signFilters[i].pop_front();
			}
			cout<<TSRSend[i]<<" ";
		}

		int end=cvGetTickCount();//测试发现颜色分割后合在一张图上速度变快
		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"  time:"<<time<<endl;
		imshow("re_src",re_src);
		waitKey(5);
		shapeResult.clear();
	}	
}
