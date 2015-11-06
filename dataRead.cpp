#include "traffic.h"

/************************************************************************/
/* Jayn 2015.11.4
/* @param XMLName:the training XML file name(with path)                                                                     */
/************************************************************************/

int HOGTrainingTrafficSign(const String path,HOGDescriptor &hog,int num_folder,int imgWidth,int imgHeight,String XMLName)
{
	stringstream SS_folder;
	String img_num,txt_path,folder,img_path;

	vector<float> pixelVector;
	vector<float> descriptors;//记录HOG特征向量
	
	Mat img,sampleFeatureMat,sampleLabelMat;
	float ClassId=0;
	int sampleNum=0;
	MySVM svm;

	//calculate the number of samples
	for (int j=0;j<num_folder;j++)
	{
		//get the folder name
		SS_folder.clear();
		SS_folder<<j;
		SS_folder>>folder;
		txt_path=path+"\\"+folder+"\\description.txt";
		ifstream txt(txt_path);
		if (!txt)
		{
			cout<<"can't open the txt file!"<<endl;
			exit(1);
		}
		//count the number of samples
		while(getline(txt,img_path))sampleNum++;
	}


	//folder ID loop
	int count_img=0;//第count_img个样本
	for(int j=0;j<num_folder;j++)
	{
		//get the folder name
		SS_folder.clear();
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
			
			//read image
			img=imread(img_path);
			Mat resizedImg(imgHeight,imgWidth,CV_8UC3) ;
			resize(img,resizedImg,resizedImg.size());

			//calculate the HOG feature,set it into descriptors
			hog.compute(resizedImg,descriptors,Size(8,8));
			cout<<"HOG Descriptor size:"<<descriptors.size()<<endl;
			int DescriptorDim= descriptors.size();
			//if it is the first time,initialize the size of Mat
			if( 0 == count_img)
			{
				sampleFeatureMat = Mat::zeros(sampleNum, DescriptorDim, CV_32FC1);
				sampleLabelMat = Mat::zeros(sampleNum, 1, CV_32FC1);
			}

			for(int i=0; i<DescriptorDim; i++)
				sampleFeatureMat.at<float>(count_img,i) = descriptors[i];//第count_img个样本的特征向量中的第i个元素
			sampleLabelMat.at<float>(count_img,0) =j;//第j个文件夹的样本的标签设置为j
			count_img++;
		}		
	}
	cout<<"sample number:"<<count_img<<endl;


	CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);
	//SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01
	CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);
	cout<<"开始训练SVM分类器"<<endl;
	svm.train_auto(sampleFeatureMat, sampleLabelMat, Mat(), Mat(), param);//训练分类器
	cout<<"训练完成"<<endl;

	svm.save(XMLName.c_str());//将训练好的SVM模型保存为xml文件
	return sampleNum;
}