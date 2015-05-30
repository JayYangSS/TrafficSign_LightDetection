#pragma once
#include"traffic.h"


struct PixelRGB{
	int r;
	int g;
	int b;
	int p_label;
};

class ClassifierTrain
{
public:
	ClassifierTrain(void);
	~ClassifierTrain(void);

	vector<PixelRGB> rgb;//存储一张图片的所有rgb
	void ClassifierTrain::getRGB(vector<Mat> &imgPosArray,vector<Mat> &imgNegArray);//将相应类别的图像和标签输入
	void train(MySVM& svm);
	void svmInfo(MySVM svm);
};



