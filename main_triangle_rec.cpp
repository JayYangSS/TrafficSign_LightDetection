#include "HOG_ANN.h"
#include "traffic.h"
#include "math_utils.h"
#include "socket_server_task.h"
#include "Tracker/CTracker.h"
#include <queue>
//void testAccuracy(String path,int num_folder);
void test_RBYcolor_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);
void test_RBYcolorMerge_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
	CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue);




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


int main()
{
	//socket通信
	SocketInit();
	g_mat = cvCreateMat(2, 1, CV_32FC1);//用于传输数据
	
	bool isTrain=false;
	CvANN_MLP nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue;
	PCA pca,pca_RoundRim,pca_RoundBlue;
	loadPCA("pcaTriangle.yml", pca);
	loadPCA("pcaRoundRim.yml", pca_RoundRim);
	loadPCA("pcaRoundBlue.yml", pca_RoundBlue);

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
	


	//test
	test_RBYcolorMerge_Video(pca,pca_RoundRim,pca_RoundBlue,nnetwork,nnetwork_RoundRim,nnetwork_RoundBlue);
	cvReleaseMat(&g_mat);
	system("pause");
}



void test_RBYcolor_Video(PCA &pca,PCA &pca_RoundRim,PCA &pca_RoundBlue,CvANN_MLP &nnetwork,
										  CvANN_MLP &nnetwork_RoundRim,CvANN_MLP &nnetwork_RoundBlue)
{
	float send=0;
	VideoCapture capture; 
	vector<Rect> boundingBox;
	vector<ShapeRecResult> shapeResult;
	Mat src,re_src,thresh;
	Scalar colorMode[]={CV_RGB(255,255,0),CV_RGB(0,0,255),CV_RGB(255,0,0)};
	//kalman tracker
	vector<Point2d> centers;
	vector<Point2d> storePredictCenters;
	Rect searchRegion;
	/*构造函数中的参数意义：
	(float _dt, float _Accel_noise_mag, double _dist_thres, int _maximum_allowed_skipped_frames,int _max_trace_length)
	时间增量（越小表示物体惯性越大），噪声幅值，_dist_thres，允许跳过的最大帧数，跟踪轨迹允许的最大长度
	*/
	CTracker tracker(0.2,0.5,60.0,10,10);
	//process every frame
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
			imshow(tmp,noiseremove);
			waitKey(2);
			//形状识别
			Mat p2=ShapeRecognize(noiseremove,shapeResult);
			//get the bounding boxes' centers
		/*	centers.clear();

			if(boundingBox.size()>0)
			{
				getCentersFromBoxes(boundingBox,centers);
				//Kalman Track
				if(centers.size()>0)
				{
					tracker.Update(centers);

					cout << tracker.tracks.size()  << endl;

					for(int i=0;i<tracker.tracks.size();i++)
					{
						if(tracker.tracks[i]->trace.size()>1)
						{
							for(int j=0;j<tracker.tracks[i]->trace.size()-1;j++)
							{
								line(re_src,tracker.tracks[i]->trace[j],tracker.tracks[i]->trace[j+1],colorMode[tracker.tracks[i]->track_id%3],2,CV_AA);
							}
						}
					}
				}
			}

			//如果这一帧没有检测到标志牌，使用Kalman Filter预测值作为搜索区域查看是否包含标志牌
			else
			{
				for (int i=0;i<tracker.tracks.size();i++)
				{
					Point2d predictCenters=tracker.tracks[i]->prediction;
					//get the search region
					searchRegion=getSearchRegion(predictCenters,Size(40,40),Size(640,480));
					float threshRatio=0.1;//如果有效像素数量比例超过阈值，则认为包含标志
					if(isContainSigns(noiseremove,searchRegion,threshRatio))
					{
						//如果目标在该预测位置内，将预测位置作为这一帧检测位置
						storePredictCenters.push_back(predictCenters);
					}
					//预测位置内不包含标志，则重新检测
					else
					{
						ShapeRecognize(noiseremove,boundingBox);
						if (boundingBox.size()>0)
						{
							getCentersFromBoxes(boundingBox,storePredictCenters);
						}
					}
				}
				if(storePredictCenters.size()!=0)
				{
					tracker.Update(storePredictCenters);
					storePredictCenters.clear();	
				}
					
			}*/


			//TODO:根据statePt点确定一个搜索区域（boundingBox: predictRegion）
		/*	if(predictRegion contains object)
			{
				measurement.at<float>(0)=foundBox.x;
				measurement.at<float>(1)=foundBox.y;
			}else{
				//全局搜索目标
				Mat p2=ShapeRecognize(noiseremove,boundingBox);
			}
			*/

			for (int i=0;i<shapeResult.size();i++)
			{
				boundingBox[i]=shapeResult[i].box;
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
	//deque<float> container[5];
	

	//构造函数中的参数意义：
	//(float _dt, float _Accel_noise_mag, double _dist_thres, int _maximum_allowed_skipped_frames,int _max_trace_length)
	//时间增量（越小表示物体惯性越大），噪声幅值，_dist_thres，允许跳过的最大帧数，跟踪轨迹允许的最大长度
	
	//CTracker tracker(0.2,0.5,60.0,10,10);
	//process every frame
	capture.open("D:\\JY\\JY_TrainingSamples\\TrafficSignVideo\\trafficSign6.avi");
	//capture.open("D:\\JY\\JY_TrainingSamples\\比赛视频截取\\signs.mp4");
	while(capture.read(src))
	{
		float send[7]={0,0,0,0,0,0,0};
		bool flag[7]={false,false,false,false,false,false,false};
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
		//形状识别
		/*Mat noiseremove;
		medianBlur(nhs_image,noiseremove,3);
		imshow("noiseremove",noiseremove);
		waitKey(2);*/

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
					//send[0]=1.0;break;
					signFilters[0].push_back(1.0);
					if (signFilters[0].size()>5)
						signFilters[0].pop_front();
					flag[0]=true;
					it=signFilters[0].begin();
					while (it<signFilters[0].end())
					{
						if(*it==1.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[0].size()>=0.4)
					{
						send[0]=1.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[0]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"man",boundingBox);
					//send[1]=2.0;break;
					signFilters[1].push_back(2.0);
					if (signFilters[1].size()>5)
						signFilters[1].pop_front();
					flag[1]=true;
					it=signFilters[1].begin();
					while (it<signFilters[1].end())
					{
						if(*it==2.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[1].size()>=0.4)
					{
						send[1]=2.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[1]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 3:
					setLabel(re_src,"slow",boundingBox);
					//send[2]=3.0;break;
					signFilters[2].push_back(3.0);
					if (signFilters[2].size()>5)
						signFilters[2].pop_front();
					flag[2]=true;
					it=signFilters[2].begin();
					while (it<signFilters[2].end())
					{
						if(*it==3.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[2].size()>=0.4)
					{
						send[2]=3.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[2]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
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
					//send[3]=4.0;break;
					signFilters[3].push_back(4.0);
					if (signFilters[3].size()>5)
						signFilters[3].pop_front();
					flag[3]=true;
					it=signFilters[3].begin();
					while (it<signFilters[3].end())
					{
						if(*it==4.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[3].size()>=0.4)
					{
						send[3]=4.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[3]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"bike",boundingBox);
					//send[4]=5.0;break;
					signFilters[4].push_back(5.0);
					if (signFilters[4].size()>5)
						signFilters[4].pop_front();
					flag[4]=true;
					it=signFilters[4].begin();
					while (it<signFilters[4].end())
					{
						if(*it==5.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[4].size()>=0.4)
					{
						send[4]=5.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[4]=0.0;
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
					//send[5]=6.0;break;
					signFilters[5].push_back(6.0);
					if (signFilters[5].size()>5)
						signFilters[5].pop_front();
					flag[5]=true;
					it=signFilters[5].begin();
					while (it<signFilters[5].end())
					{
						if(*it==6.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[5].size()>=0.4)
					{
						send[5]=6.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[5]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	

				case 2:
					setLabel(re_src,"30",boundingBox);
					signFilters[6].push_back(7.0);
					if (signFilters[6].size()>5)
						signFilters[6].pop_front();
					flag[6]=true;
					it=signFilters[6].begin();
					while (it<signFilters[6].end())
					{
						if(*it==7.0)count++;
						it++;
					}
					if((float)(count)/(float)signFilters[6].size()>=0.4)
					{
						send[6]=7.0;
						//cout<<"detected"<<endl;
					}
					else
					{
						send[6]=0.0;
						//cout<<"No detected"<<endl;
					}
					count=0;
					break;	
					//send=7.0;break;
				default:
					break;
				}
			}
			
		}

		for (int i=0;i<=6;i++)
		{
			if(!flag)
			{
				signFilters[i].push_back(0);
				if (signFilters[i].size()>5)
					signFilters[i].pop_front();
			}
			cout<<send[i]<<" ";
		}
		cout<<""<<endl;
		imshow("re_src",re_src);
		waitKey(5);
		shapeResult.clear();
	}	
}