#include "std_tlr.h"
#include "traffic.h"
extern Size Win_vertical,block_vertical,blockStride_vertical,cell_vertical;

void hogSVMTrainTL(HOGDescriptor &myHOG,bool TRAIN,bool HORZ)
{
	
	//检测窗口(64,128),块尺寸(16,16),块步长(8,8),cell尺寸(8,8),直方图bin个数9
	HOGDescriptor hog_vertical(Win_vertical,block_vertical,blockStride_vertical,cell_vertical,9,1,-1.0,0,0.2,true,64);//HOG检测器，用来计算HOG描述子的
	HOGDescriptor hog_horz(Size(36,12),Size(12,6),Size(6,6),Size(6,6),9,1,-1.0,0,0.2,true,64);

  int DescriptorDim;//HOG描述子的维数，由图片大小、检测窗口大小、块大小、细胞单元中直方图bin个数决定
  MySVM svm;//SVM分类器
  int WinStride;
  //若TRAIN为true，重新训练分类器



  if(!HORZ)
  {
	  if(TRAIN)
  {
    string ImgName;//图片名(绝对路径)
   ifstream finPos("D:\\JY\\JY_TrainingSamples\\positive\\positivePath.txt");//正样本图片的文件名列表
   ifstream finNeg("D:\\JY\\JY_TrainingSamples\\negetive1\\negetive1.txt");//负样本图片的文件名列表
   

   Mat sampleFeatureMat;//所有训练样本的特征向量组成的矩阵，行数等于所有样本的个数，列数等于HOG描述子维数	
   Mat sampleLabelMat;//训练样本的类别向量，行数等于所有样本的个数，列数等于1；1表示有人，-1表示无人
	


    //依次读取正样本图片，生成HOG描述子

    for(int num=0; num<PosSamNO && getline(finPos,ImgName); num++)
    {
		cout<<"处理："<<ImgName<<endl;
		Mat src = imread(ImgName);//读取图片

 
		//resize(src,src,Size(15,30));
		resize(src,src,Win_vertical);
		vector<float> descriptors;//HOG描述子向量
		hog_vertical.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
		cout<<"描述子维数："<<descriptors.size()<<endl;

      //处理第一个样本时初始化特征向量矩阵和类别矩阵，因为只有知道了特征向量的维数才能初始化特征向量矩阵
      if( 0 == num )
      {
        DescriptorDim = descriptors.size();//HOG描述子的维数
        //初始化所有训练样本的特征向量组成的矩阵，行数等于所有样本的个数，列数等于HOG描述子维数sampleFeatureMat
		sampleFeatureMat = Mat::zeros(PosSamNO+NegSamNO+HardExampleNO, DescriptorDim, CV_32FC1);
        sampleLabelMat = Mat::zeros(PosSamNO+NegSamNO+HardExampleNO, 1, CV_32FC1);
		//sampleLabelMat = Mat::zeros(PosSamNO+5742*NegSamNO+HardExampleNO, 1, CV_32FC1);
      }

      //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
      for(int i=0; i<DescriptorDim; i++)
		sampleFeatureMat.at<float>(num,i) = descriptors[i];//第num个样本的特征向量中的第i个元素
      sampleLabelMat.at<float>(num,0) = 1;//正样本类别为1，有人
    }
	
    //依次读取负样本图片，生成HOG描述子

    for(int num=0; num<NegSamNO && getline(finNeg,ImgName); num++)
    {
      cout<<"处理："<<ImgName<<endl;
      Mat src = imread(ImgName);//读取图片
	 
	 //resize(src,src,Size(15,30));
	 resize(src,src,Win_vertical);
	  //cvtColor(src,gray,CV_RGB2GRAY);
      vector<float> descriptors;//HOG描述子向量
	  hog_vertical.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
      cout<<"描述子维数："<<descriptors.size()<<endl;

      //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
/*	 WinStride=descriptors.size()/DescriptorDim;
	  for(int j=0;j<WinStride;j++)
	  {
		  for(int i=0; i<DescriptorDim; i++)
				sampleFeatureMat.at<float>(j+num*WinStride+PosSamNO,i) = descriptors[i+DescriptorDim*j];//第PosSamNO+num个样本的特征向量中的第i个元素
		  sampleLabelMat.at<float>(j+num*WinStride+PosSamNO,0) = -1;//负样本类别为-1，无人
	  }*/


    for(int i=0; i<DescriptorDim; i++)
        sampleFeatureMat.at<float>(num+PosSamNO,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
      sampleLabelMat.at<float>(num+PosSamNO,0) = -1;//负样本类别为-1，无人
    }

    //处理HardExample负样本
    if(HardExampleNO > 0)
    {
      ifstream finHardExample("D:\\JY\\JY_TrainingSamples\\hardexample\\hardexample.txt");//HardExample负样本的文件名列表
      //依次读取HardExample负样本图片，生成HOG描述子
      for(int num=0; num<HardExampleNO && getline(finHardExample,ImgName); num++)
      {
        cout<<"处理："<<ImgName<<endl;
       // ImgName = ImgName;//加上HardExample负样本的路径名
        Mat src = imread(ImgName);//读取图片

		//resize(src,src,Size(15,30));
		resize(src,src,Win_vertical);
		//cvtColor(src,gray,CV_RGB2GRAY);
        vector<float> descriptors;//HOG描述子向量
       hog_vertical.compute(src,descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
		//hog_vertical.compute(src,descriptors,Size(5,5));//计算HOG描述子，检测窗口移动步长(8,8)
        //cout<<"描述子维数："<<descriptors.size()<<endl;

        //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
        for(int i=0; i<DescriptorDim; i++)
          sampleFeatureMat.at<float>(num+PosSamNO+NegSamNO,i) = descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
        sampleLabelMat.at<float>(num+PosSamNO+NegSamNO,0) = -1;//负样本类别为-1，无人
      }
    }

    ////输出样本的HOG特征向量矩阵到文件
    //ofstream fout("SampleFeatureMat.txt");
    //for(int i=0; i<PosSamNO+NegSamNO; i++)
    //{
    //	fout<<i<<endl;
    //	for(int j=0; j<DescriptorDim; j++)
    //		fout<<sampleFeatureMat.at<float>(i,j)<<"  ";
    //	fout<<endl;
    //}

    //训练SVM分类器
    //迭代终止条件，当迭代满1000次或误差小于FLT_EPSILON时停止迭代
    CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);
    //SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01
    CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);
    cout<<"开始训练SVM分类器"<<endl;
    svm.train(sampleFeatureMat, sampleLabelMat, Mat(), Mat(), param);//训练分类器
    cout<<"训练完成"<<endl;
    svm.save("src\\TrafficLightDetection\\SVM_HOG_Benchmark.xml");//将训练好的SVM模型保存为xml文件
	//svm.save("SVM_HOG_10_20.xml");
  }
  else //若TRAIN为false，从XML文件读取训练好的分类器
  {
    svm.load("src\\TrafficLightDetection\\SVM_HOG_Benchmark.xml");//从XML文件读取训练好的SVM模型
	// svm.load("SVM_HOG_10_20.xml");
  }


  /*************************************************************************************************
  线性SVM训练完成后得到的XML文件里面，有一个数组，叫做support vector，还有一个数组，叫做alpha,有一个浮点数，叫做rho;
  将alpha矩阵同support vector相乘，注意，alpha*supportVector,将得到一个列向量。之后，再该列向量的最后添加一个元素rho。
  如此，变得到了一个分类器，利用该分类器，直接替换opencv中行人检测默认的那个分类器（cv::HOGDescriptor::setSVMDetector()），
  就可以利用你的训练样本训练出来的分类器进行行人检测了。
  ***************************************************************************************************/
  DescriptorDim = svm.get_var_count();//特征向量的维数，即HOG描述子的维数
  int supportVectorNum = svm.get_support_vector_count();//支持向量的个数
 // cout<<"支持向量个数："<<supportVectorNum<<endl;

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

  //计算-(alphaMat * supportVectorMat),结果放到resultMat中
  //gemm(alphaMat, supportVectorMat, -1, 0, 1, resultMat);//不知道为什么加负号？
  resultMat = -1 * alphaMat * supportVectorMat;

  //得到最终的setSVMDetector(const vector<float>& detector)参数中可用的检测子
  vector<float> myDetector;
  //将resultMat中的数据复制到数组myDetector中
  for(int i=0; i<DescriptorDim; i++)
  {
    myDetector.push_back(resultMat.at<float>(0,i));
  }
  //最后添加偏移量rho，得到检测子
  myDetector.push_back(svm.get_rho());
 // cout<<"检测子维数："<<myDetector.size()<<endl;
  //设置HOGDescriptor的检测子
 // HOGDescriptor myHOG_vertical(Size(15,30),Size(5,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,34);//此处若不加括号中的参数会采用opencv中默认的HOG检测子参数，会与之前设定的参数矛盾，纠结了一下午！！！
  myHOG.setSVMDetector(myDetector);//传入hog_vertical.cpp中的setSVMDetector函数中的svmDetector中
  //myDetector.push_back(svm.get_rho());/////////////////////////////////daigai
  //myHOG_vertical.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());

  //保存检测子参数到文件
  ofstream fout("src\\HOGDetectorForOpenCV_BenchMark.txt");
  for(int i=0; i<myDetector.size(); i++)
  {
    fout<<myDetector[i]<<endl;
  }
  }





  else
{
	if(TRAIN)
  {
    string horz_ImgName;//图片名(绝对路径)
    ifstream horz_finPos("D:\\Traffic Light Detection\\JY_TrainingSamples\\horz_positive\\horz_Pos.txt");
    ifstream horz_finNeg("D:\\Traffic Light Detection\\JY_TrainingSamples\\horz_Neg\\horz_Neg.txt");
	// ifstream finNeg("C:\\Users\\JY\\Desktop\\test\\test.txt");//负样本图片的文件名列表
   
	
	Mat Horz_sampleFeatureMat;//所有训练样本的特征向量组成的矩阵，行数等于所有样本的个数，列数等于HOG描述子维数	
    Mat Horz_sampleLabelMat;//训练样本的类别向量，行数等于所有样本的个数，列数等于1；1表示有人，-1表示无人

    //依次读取正样本图片，生成HOG描述子

    for(int num=0; num<HORZ_PosSamNO && getline(horz_finPos,horz_ImgName); num++)
    {
		cout<<"处理："<<horz_ImgName<<endl;
      Mat horz_src = imread(horz_ImgName);//读取图片

   
	  resize(horz_src,horz_src,Size(30,15));
	//resize(horz_src,horz_src,Size(50,20));
	

      vector<float> horz_descriptors;//HOG描述子向量
      hog_horz.compute(horz_src,horz_descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
	 // hog_vertical.compute(src,descriptors,Size(5,5));//计算HOG描述子，检测窗口移动步长(8,8)
      cout<<"描述子维数："<<horz_descriptors.size()<<endl;

      //处理第一个样本时初始化特征向量矩阵和类别矩阵，因为只有知道了特征向量的维数才能初始化特征向量矩阵
      if( 0 == num )
      {
        DescriptorDim = horz_descriptors.size();//HOG描述子的维数
        //初始化所有训练样本的特征向量组成的矩阵，行数等于所有样本的个数，列数等于HOG描述子维数sampleFeatureMat
        Horz_sampleFeatureMat = Mat::zeros(HORZ_PosSamNO+HORZ_NegSamNO+HORZ_HardExampleNO, DescriptorDim, CV_32FC1);
		//sampleFeatureMat = Mat::zeros(PosSamNO+5742*NegSamNO+HardExampleNO, DescriptorDim, CV_32FC1);
        //初始化训练样本的类别向量，行数等于所有样本的个数，列数等于1；1表示有人，0表示无人
        Horz_sampleLabelMat = Mat::zeros(HORZ_PosSamNO+HORZ_NegSamNO+HORZ_HardExampleNO, 1, CV_32FC1);
		//sampleLabelMat = Mat::zeros(PosSamNO+5742*NegSamNO+HardExampleNO, 1, CV_32FC1);
      }

      //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
      for(int i=0; i<DescriptorDim; i++)
		Horz_sampleFeatureMat.at<float>(num,i) = horz_descriptors[i];//第num个样本的特征向量中的第i个元素
        Horz_sampleLabelMat.at<float>(num,0) = 1;//正样本类别为1，有人
    }
	
    //依次读取负样本图片，生成HOG描述子

    for(int num=0; num<HORZ_NegSamNO && getline(horz_finNeg,horz_ImgName); num++)
    {
      cout<<"处理："<<horz_ImgName<<endl;
      Mat horz_src = imread(horz_ImgName);//读取图片
	 
      //resize(src,src,Size(64,128));
	 resize(horz_src,horz_src,Size(30,15));
	   //resize(horz_src,horz_src,Size(50,20));
	  //cvtColor(src,gray,CV_RGB2GRAY);
      vector<float> horz_descriptors;//HOG描述子向量
	  hog_horz.compute(horz_src,horz_descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
      cout<<"描述子维数："<<horz_descriptors.size()<<endl;

      

    for(int i=0; i<DescriptorDim; i++)
        Horz_sampleFeatureMat.at<float>(num+HORZ_PosSamNO,i) = horz_descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
		Horz_sampleLabelMat.at<float>(num+HORZ_PosSamNO,0) = -1;//负样本类别为-1，无人
    }

    //处理HardExample负样本
    if(HORZ_HardExampleNO > 0)
    {
      ifstream horz_finHardExample("D:\\Traffic Light Detection\\JY_TrainingSamples\\horz_HardExample\\hardexample.txt");//HardExample负样本的文件名列表
      //依次读取HardExample负样本图片，生成HOG描述子
      for(int num=0; num<HORZ_HardExampleNO && getline(horz_finHardExample,horz_ImgName); num++)
      {
        cout<<"处理："<<horz_ImgName<<endl;
       // ImgName = ImgName;//加上HardExample负样本的路径名
        Mat horz_src = imread(horz_ImgName);//读取图片

		
		//resize(src,src,Size(64,128));
		resize(horz_src,horz_src,Size(30,15));
		//resize(src,src,Size(10,20));
		//cvtColor(src,gray,CV_RGB2GRAY);
        vector<float> horz_descriptors;//HOG描述子向量
        hog_horz.compute(horz_src,horz_descriptors,Size(8,8));//计算HOG描述子，检测窗口移动步长(8,8)
		//hog_vertical.compute(src,descriptors,Size(5,5));//计算HOG描述子，检测窗口移动步长(8,8)
        //cout<<"描述子维数："<<descriptors.size()<<endl;

        //将计算好的HOG描述子复制到样本特征矩阵sampleFeatureMat
        for(int i=0; i<DescriptorDim; i++)
          Horz_sampleFeatureMat.at<float>(num+HORZ_PosSamNO+HORZ_NegSamNO,i) = horz_descriptors[i];//第PosSamNO+num个样本的特征向量中的第i个元素
		  Horz_sampleLabelMat.at<float>(num+HORZ_PosSamNO+HORZ_NegSamNO,0) = -1;//负样本类别为-1，无人
      }
    }

    ////输出样本的HOG特征向量矩阵到文件
    //ofstream fout("SampleFeatureMat.txt");
    //for(int i=0; i<PosSamNO+NegSamNO; i++)
    //{
    //	fout<<i<<endl;
    //	for(int j=0; j<DescriptorDim; j++)
    //		fout<<sampleFeatureMat.at<float>(i,j)<<"  ";
    //	fout<<endl;
    //}

    //训练SVM分类器
    //迭代终止条件，当迭代满1000次或误差小于FLT_EPSILON时停止迭代
    CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);
    //SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01
    CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);
    cout<<"开始训练SVM分类器"<<endl;
    svm.train(Horz_sampleFeatureMat, Horz_sampleLabelMat, Mat(), Mat(), param);//训练分类器
    cout<<"训练完成"<<endl;
    svm.save("src\\TrafficLightDetection\\SVM_HOG_Horz.xml");//将训练好的SVM模型保存为xml文件

  }
  else //若TRAIN为false，从XML文件读取训练好的分类器
  {
    svm.load("src\\TrafficLightDetection\\SVM_HOG_Horz.xml");//从XML文件读取训练好的SVM模型
  }


  /*************************************************************************************************
  线性SVM训练完成后得到的XML文件里面，有一个数组，叫做support vector，还有一个数组，叫做alpha,有一个浮点数，叫做rho;
  将alpha矩阵同support vector相乘，注意，alpha*supportVector,将得到一个列向量。之后，再该列向量的最后添加一个元素rho。
  如此，变得到了一个分类器，利用该分类器，直接替换opencv中行人检测默认的那个分类器（cv::HOGDescriptor::setSVMDetector()），
  就可以利用你的训练样本训练出来的分类器进行行人检测了。
  ***************************************************************************************************/
  DescriptorDim = svm.get_var_count();//特征向量的维数，即HOG描述子的维数
  int supportVectorNum = svm.get_support_vector_count();//支持向量的个数
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

  //计算-(alphaMat * supportVectorMat),结果放到resultMat中
  //gemm(alphaMat, supportVectorMat, -1, 0, 1, resultMat);//不知道为什么加负号？
  resultMat = -1 * alphaMat * supportVectorMat;

  //得到最终的setSVMDetector(const vector<float>& detector)参数中可用的检测子
  vector<float> myDetector;
  //将resultMat中的数据复制到数组myDetector中
  for(int i=0; i<DescriptorDim; i++)
  {
    myDetector.push_back(resultMat.at<float>(0,i));
  }
  //最后添加偏移量rho，得到检测子
  myDetector.push_back(svm.get_rho());
  cout<<"检测子维数："<<myDetector.size()<<endl;
  //设置HOGDescriptor的检测子
 // HOGDescriptor myHOG_vertical(Size(15,30),Size(5,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,34);//此处若不加括号中的参数会采用opencv中默认的HOG检测子参数，会与之前设定的参数矛盾，纠结了一下午！！！
  myHOG.setSVMDetector(myDetector);//传入hog_vertical.cpp中的setSVMDetector函数中的svmDetector中
  //myDetector.push_back(svm.get_rho());/////////////////////////////////daigai
  //myHOG_vertical.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());

  //保存检测子参数到文件
  ofstream fout("src\\TrafficLightDetection\\HOGDetectorForOpenCV_horz.txt");
  for(int i=0; i<myDetector.size(); i++)
  {
    fout<<myDetector[i]<<endl;
  }
}

  

}