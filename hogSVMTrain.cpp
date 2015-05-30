#include"traffic.h"
void hogSVMTrain( HOGDescriptor &myHOG,bool TRAIN)
{
	HOGDescriptor hog(Size(20,20),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,30);
    int DescriptorDim;
    MySVM svm;
    int WinStride;
 

  if(TRAIN)
  {
    string ImgName;
    ifstream finPos("D:\\VS2010_Projects\\JY_TrafficLight\\HOG_Traffic\\positive\\positivePath.txt");//正样本图片的文件名列表
	ifstream finNeg("D:\\VS2010_Projects\\JY_TrafficLight\\HOG_Traffic\\N_Neg\\N_Neg.txt");//负样本图片的文件名列表
    Mat sampleFeatureMat;
    Mat sampleLabelMat;
	


    //load positive samples and compute hog descriptor
    for(int num=0; num<PosSamNO && getline(finPos,ImgName); num++)
    {
		cout<<"处理："<<ImgName<<endl;
		Mat src = imread(ImgName);
 		resize(src,src,Size(15,30));
		vector<float> descriptors;//HOG descriptor
		hog.compute(src,descriptors,Size(8,8));//block stride(8,8)
		cout<<"描述子维数："<<descriptors.size()<<endl;

        //initialize the first mat
        if( 0 == num )
        {
          DescriptorDim = descriptors.size();//HOG描述子的维数
		  sampleFeatureMat = Mat::zeros(PosSamNO+NegSamNO+HardExampleNO, DescriptorDim, CV_32FC1);
          sampleLabelMat = Mat::zeros(PosSamNO+NegSamNO+HardExampleNO, 1, CV_32FC1);
        }


        for(int i=0; i<DescriptorDim; i++)
		  sampleFeatureMat.at<float>(num,i) = descriptors[i];//第num个样本的特征向量中的第i个元素
        sampleLabelMat.at<float>(num,0) = 1;//正样本类别为1，有人
     }
	




	//load negative samples and compute hog descriptor
     for(int num=0; num<NegSamNO && getline(finNeg,ImgName); num++)
     {
       cout<<"处理："<<ImgName<<endl;
       Mat src = imread(ImgName);//读取图
	   resize(src,src,Size(15,30));
       vector<float> descriptors;//HOG描述子向量
	   hog.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
       cout<<"描述子维数："<<descriptors.size()<<endl;


      //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
       for(int i=0; i<DescriptorDim; i++)
         sampleFeatureMat.at<float>(num+PosSamNO,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
       sampleLabelMat.at<float>(num+PosSamNO,0) = -1;//负样本类别为-1，无人
     }

    //处理HardExample负样本
    if(HardExampleNO > 0)
    {
      ifstream finHardExample("D:\\VS2010_Projects\\JY_TrafficLight\\HOG_Traffic\\hardexample\\hardexample.txt");//HardExample负样本的文件名列表
      //依次读取HardExample负样本图片，生成HOG描述子
      for(int num=0; num<HardExampleNO && getline(finHardExample,ImgName); num++)
      {
        cout<<"处理："<<ImgName<<endl;
       // ImgName = ImgName;//加上HardExample负样本的路径名
        Mat src = imread(ImgName);//读取图片
		resize(src,src,Size(15,30));
        vector<float> descriptors;//HOG描述子向量
        hog.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)

        //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
        for(int i=0; i<DescriptorDim; i++)
          sampleFeatureMat.at<float>(num+PosSamNO+NegSamNO,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
        sampleLabelMat.at<float>(num+PosSamNO+NegSamNO,0) = -1;//负样本类别为-1，无人
      }
    }


    //训练SVM分类器,迭代终止条件，当迭代满1000次或误差小于FLT_EPSILON时停止迭代
    CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);
    //SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01
    CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);
    cout<<"开始训练SVM分类器"<<endl;
    svm.train(sampleFeatureMat, sampleLabelMat, Mat(), Mat(), param);//训练分类器
    cout<<"训练完成"<<endl;
    svm.save("SVM_HOG.xml");//将训练好的SVM模型保存为xml文件

  }
  else //若TRAIN为false，从XML文件读取训练好的分类器
  {
    svm.load("SVM_HOG_BenchMark.xml");//从XML文件读取训练好的SVM模型
  }


  //train SVM
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