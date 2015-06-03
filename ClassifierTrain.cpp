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


void ClassifierTrain::getRGB(vector<Mat> &imgPosArray,vector<Mat> &imgNegArray)
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
					pixel_rgb.p_label=POS_LABEL;
					rgb.push_back(pixel_rgb);
				}
			}
		}
	}
	

	//read negative samples
	for(iter=imgNegArray.begin();iter!=imgNegArray.end();iter++)
	{
		Mat img=(*iter);
		if(img.channels()!=3)
		{
			cout<<"The input negative samples are not 3 channels!"<<endl;
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
					pixel_rgb.p_label=NEG_LABEL;
					rgb.push_back(pixel_rgb);
				}
			}
		}
	}
	
}



void ClassifierTrain::train(bool isTrain)
{
	if(isTrain)//isTrain=true,进行训练
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
		svm.save("src//SVM_HOG_color.xml");
	}
	else
		svm.load("src//SVM_HOG_color.xml");
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
				SegImg.at<float>(j,i/3)=255;
			}
			else{
				SegImg.at<float>(j,i/3)=0;
			}
		}
	}
	return SegImg;
}
