#include "ClassifierTrain.h"
#define POS_LABEL 1
#define NEG_LABEL -1

//#include "traffic.h"
ClassifierTrain::ClassifierTrain(void)
{
}


ClassifierTrain::~ClassifierTrain(void)
{
}


void ClassifierTrain::getRGB(vector<Mat> &imgPosArray,vector<PixelRGB> &rgb,float label)
{
	//read positive samples
	vector<Mat>::iterator iter;
	for(iter=imgPosArray.begin();iter!=imgPosArray.end();iter++)
	{
		Mat img=(*iter);
		if(img.channels()!=3)
		{
			cout<<"The input positive samples are not 3 channels!"<<endl;
			exit(0);
		}else{
			int nr=img.rows;
			int nc=img.cols;
			PixelRGB pixel_rgb;

			//scan the pixel
			for (int j=0;j<nr;j++)
			{
				uchar* data=img.ptr<uchar>(j);
				for (int i=0;i<3*nc;i=i+3)
				{
					pixel_rgb.b=data[i];
					pixel_rgb.g=data[i+1];
					pixel_rgb.r=data[i+2];
					pixel_rgb.p_label=label;
					rgb.push_back(pixel_rgb);
				}
			}
		}
	}
	
}





void ClassifierTrain::train(vector<PixelRGB> &rgb)
{
		int rows=rgb.size();//number of pixels
		Mat rgbFeature=Mat::zeros(rows,3, CV_32FC1);//save the rgb information 
		Mat rgbLabel=Mat::zeros(rows,1, CV_32FC1);//save the label information
		//存入Mat中
		for (int j=0;j<rows;j++)
		{
			rgbFeature.at<float>(j,0)=rgb[j].b;
			rgbFeature.at<float>(j,1)=rgb[j].g;
			rgbFeature.at<float>(j,2)=rgb[j].r;
			rgbLabel.at<float>(j,0)=rgb[j].p_label;
		}


		CvTermCriteria criteria = cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, FLT_EPSILON);//训练SVM分类器,迭代终止条件，当迭代满1000次或误差小于FLT_EPSILON时停止迭代
		CvSVMParams param(CvSVM::C_SVC, CvSVM::LINEAR, 0, 1, 0, 0.01, 0, 0, 0, criteria);//SVM参数：SVM类型为C_SVC；线性核函数；松弛因子C=0.01

		cout<<"开始训练SVM分类器"<<endl;
		svm.train(rgbFeature,rgbLabel, Mat(), Mat(), param);
		cout<<"训练完成"<<endl;
		svm.save("src//SVM_HOG_color_multi.xml");
}


void ClassifierTrain::svmInfo()
{
	int DescriptorDim = svm.get_var_count();
	int supportVectorNum = svm.get_support_vector_count();
	cout<<"number of support vector："<<supportVectorNum<<endl;
	cout<<"Dimension of svm:"<<DescriptorDim<<endl;
}



//TODO:修改svm参数，实现多颜色分类
Mat ClassifierTrain::colorThreshold(Mat img)
{
	
	int nr=img.rows;
	int nc=img.cols;
	Mat temp_pixel=Mat::zeros(1,3,CV_32FC1);
	Mat SegImg=Mat::zeros(img.size(),CV_32FC1);
	
	if (nr!=SegImg.rows||nc!=SegImg.cols)
	{
		cout<<"size of test image and segImg does not match!"<<endl;
		exit(0);
	}
	for(int j=0;j<nr;j++)
	{
		uchar* data=img.ptr<uchar>(j);
		for (int i=0;i<3*nc;i=i+3)
		{
			temp_pixel.at<float>(0,0)=data[i];
			temp_pixel.at<float>(0,1)=data[i+1];
			temp_pixel.at<float>(0,2)=data[i+2];
			float response=svm.predict(temp_pixel);
			if (response==1.0)
			{
				SegImg.at<float>(j,i/3)=1;
			}
			/*else if (response==2.0)
			{
				SegImg.at<float>(j,i/3)=0.3;
			}*/
			else if (response==3.0)
			{
				SegImg.at<float>(j,i/3)=0.7;
			}
			else{
				SegImg.at<float>(j,i/3)=0;
			}
		}
	}
	return SegImg;
}


//训练得到该类的svm
void ClassifierTrain::TrainSVM(bool isTrain)
{
	//isTrain=true,进行训练
	if(isTrain)
	{
		vector<PixelRGB> rgb_r,rgb_b,rgb_y,rgb_n;
		char redPath[200];
		char bluePath[200];
		char yellowPath[200];
		char negPath[200];
		int numRed=4,numBlue=3,numNeg=3;
	
		//read the red samples
		vector<Mat> p_red;
		for (int i=0;i<numRed;i++)
		{
			sprintf_s(redPath,"D:\\JY\\JY_TrainingSamples\\color\\red\\%d.jpg",i);
			Mat p=imread(redPath);
			p_red.push_back(p);
		}


		//read the blue samples
	/*	vector<Mat> p_blue;
		for (int i=0;i<numBlue;i++)
		{
			sprintf_s(bluePath,"D:\\JY\\JY_TrainingSamples\\color\\blue\\%d.jpg",i);
			Mat p=imread(bluePath);
			p_blue.push_back(p);
		}*/

		//read the yellow samples
		vector<Mat> p_yellow;
		for (int i=0;i<7;i++)
		{
			sprintf_s(yellowPath,"D:\\JY\\JY_TrainingSamples\\color\\yellow\\%d.jpg",i);
			Mat p=imread(yellowPath);
			p_yellow.push_back(p);
		}


		//read the negative samples
		vector<Mat> p_neg;
		for (int i=0;i<numNeg;i++)
		{
			sprintf_s(negPath,"D:\\JY\\JY_TrainingSamples\\color\\negative\\%d.jpg",i);
			Mat p=imread(negPath);
			p_neg.push_back(p);
		}



		getRGB(p_red,rgb_r,1.0);
	//	getRGB(p_blue,rgb_b,2.0);
		getRGB(p_yellow,rgb_y,3.0);
		getRGB(p_neg,rgb_n,-1.0);
		rgb_r.insert(rgb_r.end(),rgb_b.begin(),rgb_b.end());
		rgb_r.insert(rgb_r.end(),rgb_y.begin(),rgb_y.end());
		rgb_r.insert(rgb_r.end(),rgb_n.begin(),rgb_n.end());
		train(rgb_r);
	}
	else
		svm.load("src//SVM_HOG_color_multi.xml");
	
}