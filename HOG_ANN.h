#ifndef _HOG_ANN_H_
#define _HOG_ANN_H_

#include <stdlib.h>
#include "opencv2/opencv.hpp"

#include <string.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
//#include <time.h>
#define IMG_NEW_DIM     40
//for the Parking Sign
#define RECT_SIGN_WIDTH  30
#define RECT_SIGN_HEIGHT  50
#define RESIZED_IMG_DIM     1764
//number of classes
#define TRIANGLE_CLASSES 3 //the number of triangle classes,include the negetive samples
#define ROUNDRIM_CLASSES 3//the number of round rim classes,include the negetive samples
#define RECTBLUE_CLASSES 2//the number of blue rect classes,include the negetive samples



//parameters of neural network
#define HIDDEN_1   40
#define HIDDEN_2   45
//#define ATTRIBUTES 23
using namespace std;
using namespace cv; 

void NeuralNetTrain(string shufflePath,string Neural_output,PCA &pca,int trainingSampleNum,int numClasses);
void loadPCA(const string &file_name,cv::PCA& pca_);
int Recognize(CvANN_MLP &nnetwork,PCA &pca,Mat test_img,int numClasses);

#endif
