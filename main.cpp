#define MAX_OUTPUT_STREAM_SIZE 256
#define RST_LENGTH (5) //统计结果的长度
#define RST_NUM (2) 
#define IMG_NEW_DIM 20

char rxbuf[MAX_OUTPUT_STREAM_SIZE];
#include"traffic.h"
#include "ClassifierTrain.h"


int main()
{
	vector<Rect> found_filtered;
	bool TRAIN=false;
	int bytes_recv = 0;
	MySVM svm;


	HOGDescriptor myHOG(Size(20,20),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
	Mat src,temp,re_src;
//	deque<int> resultR_static,resultN_static;//用来滤波，使结果更稳定准确
	VideoCapture capture; 

	//train
	hogSVMTrain(svm,TRAIN);

	string csvFile="D:\\JY\\JY_TrainingSamples\\GTSRB_Final_Training_Images\\GTSRB\\Final_Training\\Images\\00002\\GT-00002.csv";
	ifstream file(csvFile.c_str());
	string line;
	int numeroLigne = 0;

	int right=0;
	//read every single image
	while(getline(file,line))
	{
		numeroLigne++;
		if(numeroLigne==1)continue;
		replace(line.begin(),line.end(),';',' ');

		istringstream iss(line);
		string rawInfo[8];
		string cell;
		int k=0;

		string imagePath;
		int RoiX1 ,  RoiY1, RoiX2, RoiY2, ClassId; 

		while (iss >> cell)
		{
			rawInfo[k] = cell;
			k++;				
		}

		imagePath =  "D:\\JY\\JY_TrainingSamples\\GTSRB_Final_Training_Images\\GTSRB\\Final_Training\\Images\\00002\\"  +  rawInfo[0] ;
		RoiX1 = atoi(rawInfo[3].c_str());
		RoiY1 = atoi(rawInfo[4].c_str());
		RoiX2 = atoi(rawInfo[5].c_str());
		RoiY2 = atoi(rawInfo[6].c_str());
		ClassId = atoi(rawInfo[7].c_str());

		src=imread( imagePath.c_str() , CV_LOAD_IMAGE_COLOR );
		Mat resizedImg(IMG_NEW_DIM,IMG_NEW_DIM,CV_8UC3);
		resize(src,resizedImg,Size(IMG_NEW_DIM,IMG_NEW_DIM));


		vector<float> descriptors;//HOG描述子向量
		myHOG.compute(resizedImg,descriptors,Size(8,8));


		int DescriptorDim=descriptors.size();
		Mat sampleFeatureMat = Mat::zeros(1, DescriptorDim, CV_32FC1);
		for(int i=0; i<DescriptorDim; i++)
			sampleFeatureMat.at<float>(0,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素

		float response=svm.predict(sampleFeatureMat);
		if(response==2.0)
			right++;	


		switch((int)(response))
		{
		case 1:
			cout<<"speed limit:30"<<endl;
			break;
		case 2:
			cout<<"speed limit:50"<<endl;
			break;
		case 3:
			cout<<"speed limit:60"<<endl;
			break;
		case 4:
			cout<<"speed limit:70"<<endl;
			break;
		case 5:
			cout<<"speed limit:80"<<endl;
			break;
		default:
			cout<<"Not a traffic sign"<<endl;
		}
	}
	float precision=((float)(right))/((float)(numeroLigne));
	cout<<"precision="<<precision<<endl;
	

	
	



	//start 
	/*capture.open("D:\\JY\\JY_TrainingSamples\\huanhu_clip2.avi");
	while(capture.read(src))
	{
		int start=cvGetTickCount();
		resize(src,re_src,Size(640,480));
		found_filtered.clear();
		BoxDetect(re_src,myHOG,found_filtered);
		int end=cvGetTickCount();


		float time=(float)(end-start)/(cvGetTickFrequency()*1000000);
		cout<<"时间："<<time<<endl;
	}	
	*/
	system("pause");
	return 0;

}



