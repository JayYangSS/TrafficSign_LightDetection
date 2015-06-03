#include"traffic.h"
#include <sys/stat.h>
#define NUM_OF_CLASSES 2
#define IMG_NEW_DIM 20

void LoadImg2HOG_Mat(string &rowPath,float Label,Mat &sampleFeatureMat,Mat &sampleLabelMat);
void LoadImg2HOG_Neg(string &rowPath,float Label,Mat &sampleFeatureMat,Mat &sampleLabelMat);
static bool fileExists(const char * fileName);
string convertInt(int number, char * prefix);

void hogSVMTrain( HOGDescriptor &myHOG,bool TRAIN)
{
	
    int DescriptorDim;
    MySVM svm;
    int WinStride;
    String posPath="D:\\JY\\JY_TrainingSamples\\GTSRB_Final_Training_Images\\GTSRB\\Final_Training\\Images";
	String negPath="D:\\JY\\JY_TrainingSamples\\negetive1\\negetive1.txt";

  if(TRAIN)
  {
    string ImgName;
    Mat sampleFeatureMat1,sampleFeatureMat2;
    Mat sampleLabelMat1,sampleLabelMat2;
	

    //load positive samples and compute hog descriptor
    LoadImg2HOG_Mat(posPath,1.0,sampleFeatureMat1,sampleLabelMat1);

	//load negative samples and compute hog descriptor
    LoadImg2HOG_Neg(negPath,-1.0,sampleFeatureMat2,sampleLabelMat2);

   sampleFeatureMat1.push_back(sampleFeatureMat2);
   sampleLabelMat1.push_back(sampleLabelMat2);

    //训练SVM分类器,迭代终止条件，当迭代满1000次或误差小于FLT_EPSILON时停止迭代
    CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);
    //SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01
    CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);
    cout<<"开始训练SVM分类器"<<endl;
    svm.train(sampleFeatureMat1, sampleLabelMat1, Mat(), Mat(), param);//训练分类器
    cout<<"训练完成"<<endl;
    svm.save("SVM_HOG.xml");//将训练好的SVM模型保存为xml文件
  }
  else //若TRAIN为false，从XML文件读取训练好的分类器
  {
    svm.load("SVM_HOG.xml");//从XML文件读取训练好的SVM模型
  }


  
  DescriptorDim = svm.get_var_count();
  int supportVectorNum = svm.get_support_vector_count();
  cout<<"支持向量个数："<<supportVectorNum<<endl;

  Mat alphaMat = Mat::zeros(1, supportVectorNum, CV_32FC1);//alpha向量，长度等于支持向量个数
  Mat supportVectorMat = Mat::zeros(supportVectorNum, DescriptorDim, CV_32FC1);//支持向量矩阵
  Mat resultMat = Mat::zeros(1, DescriptorDim, CV_32FC1);//alpha向量乘以支持向量矩阵的结果

  //将支持向量的数据复制到supportVectorMat矩阵中
  for(int i=0; i<supportVectorNum; i++)
  {
    const float * pSVData = svm.get_support_vector(i);//返回第i个支持向量的数据指针
    for(int j=0; j<DescriptorDim; j++)
    {
      //cout<<pData[j]<<" ";
      supportVectorMat.at<float>(i,j) = pSVData[j];
    }
  }

  //将alpha向量的数据复制到alphaMat中
  double * pAlphaData = svm.get_alpha_vector();//返回SVM的决策函数中的alpha向量
  for(int i=0; i<supportVectorNum; i++)
  {
    alphaMat.at<float>(0,i) = pAlphaData[i];
  }

  resultMat = -1 * alphaMat * supportVectorMat;
  vector<float> myDetector;
  for(int i=0; i<DescriptorDim; i++)
  {
    myDetector.push_back(resultMat.at<float>(0,i));
  }




  //最后添加偏移量rho，得到检测子
  myDetector.push_back(svm.get_rho());
  cout<<"检测子维数："<<myDetector.size()<<endl;
 

  myHOG.setSVMDetector(myDetector);//传入hog.cpp中的setSVMDetector函数中的svmDetector中
  ofstream fout("HOGDetectorForOpenCV.txt");
  for(int i=0; i<myDetector.size(); i++)
  {
    fout<<myDetector[i]<<endl;
  }


}




string convertInt(int number, char * prefix)
{
	stringstream ss;//create a stringstream
	ss << prefix << number;//add number to the stream
	return ss.str();//return a string with the contents of the stream
}

static bool fileExists(const char * fileName)
{
	struct stat stFileInfo;
	const int intStat = stat(fileName, &stFileInfo);
	return (intStat == 0);
}



//依次读取正样本图片，将样本的HOG特征和标签分别存入sampleFeatureMat和sampleLabelMat中
void LoadImg2HOG_Mat(string &rowPath,float Label,Mat &sampleFeatureMat,Mat &sampleLabelMat)
{
	int DescriptorDim;
	HOGDescriptor hog(Size(IMG_NEW_DIM,IMG_NEW_DIM),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
	String trainingSample[NUM_OF_CLASSES] = {"00","01"}; 
	int num=0;
	Mat croppedImg,img;
	Mat resizedImg(IMG_NEW_DIM,IMG_NEW_DIM,CV_8UC3);
	Mat tempFeatureMat;
	Mat tempLabelMat;
	//read every directory 
	for (int i=0;i<NUM_OF_CLASSES;i++)
	{
		string numFolder= "000"+trainingSample[i];
		string folder=rowPath+"\\"+numFolder;
		string csvFile=folder+"\\"+"GT-"+numFolder+".csv";



		ifstream file(csvFile.c_str());
		string line;
		int numeroLigne = 0;

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

			imagePath =  folder + "\\"  +  rawInfo[0] ;
			RoiX1 = atoi(rawInfo[3].c_str());
			RoiY1 = atoi(rawInfo[4].c_str());
			RoiX2 = atoi(rawInfo[5].c_str());
			RoiY2 = atoi(rawInfo[6].c_str());
			ClassId = atoi(rawInfo[7].c_str());

			img = imread( imagePath.c_str() , CV_LOAD_IMAGE_COLOR );
			Rect ROI(RoiX1, RoiY1, RoiX2 - RoiX1, RoiY2 - RoiY1);
			croppedImg= img(ROI).clone();
			resize(croppedImg ,resizedImg ,resizedImg.size());
		

			vector<float> descriptors;//HOG descriptor
			hog.compute(resizedImg,descriptors,Size(8,8));//block stride(8,8)
			cout<<"描述子维数："<<descriptors.size()<<endl;

			DescriptorDim = descriptors.size();//HOG描述子的维数
			//initialize the first mat
			if( 0 == num )
			{
				sampleFeatureMat.create(1,DescriptorDim,CV_32FC1);  
				sampleLabelMat.create(1,1,CV_32FC1); 
				for(int i=0;i<DescriptorDim;i++)
					sampleFeatureMat.at<float>(num,i) = descriptors[i];//第num个样本的特征向量中的第i个元素
				 sampleLabelMat.at<float>(0,0) = Label;//负样本类别为-1，无人
			}else{
				tempFeatureMat=Mat::zeros(1,DescriptorDim,CV_32FC1);
				tempLabelMat=Mat::zeros(1,1,CV_32FC1);

				
				for(int i=0;i<DescriptorDim;i++)
					tempFeatureMat.at<float>(0,i) = descriptors[i];//第num个样本的特征向量中的第i个元素
				tempLabelMat.at<float>(0,0) = Label;//负样本类别为-1，无人

				//concatenate the feature and label mat of every single image into one Mat
				sampleFeatureMat.push_back(tempFeatureMat);
				sampleLabelMat.push_back(tempLabelMat);
			}
			num++;
			cout<<num<<endl;
		}
		file.close();
	}	
}



//依次读取负样本图片，将样本的HOG特征和标签分别存入sampleFeatureMat和sampleLabelMat中
void LoadImg2HOG_Neg(string &rowPath,float Label,Mat &sampleFeatureMat,Mat &sampleLabelMat)
{
	int DescriptorDim; 
	ifstream finNeg(rowPath);//负样本图片的文件名列表
	string ImgName;//图片名(绝对路径)
	HOGDescriptor hog(Size(IMG_NEW_DIM,IMG_NEW_DIM),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
	

    for(int num=0; num<NegSamNO && getline(finNeg,ImgName); num++)
    {
      cout<<"处理："<<ImgName<<endl;
      Mat src = imread(ImgName);//读取图片
	  resize(src,src,Size(IMG_NEW_DIM,IMG_NEW_DIM));
      vector<float> descriptors;//HOG描述子向量
	  hog.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
      cout<<"描述子维数："<<descriptors.size()<<endl;
	  DescriptorDim = descriptors.size();//HOG描述子的维数


	  if(num==0)
	  {
		  sampleFeatureMat = Mat::zeros(NegSamNO, DescriptorDim, CV_32FC1);
		  sampleLabelMat = Mat::zeros(NegSamNO, 1, CV_32FC1);
	  }
	  

	  for(int i=0; i<DescriptorDim; i++)
			sampleFeatureMat.at<float>(num,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
		sampleLabelMat.at<float>(num,0) = -1;//负样本类别为-1，无人
	}
	finNeg.close();

}