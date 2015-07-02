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
#include <time.h>
#define IMG_NEW_DIM     40
#define RESIZED_IMG_DIM     1764
//number of classes
#define TRIANGLE_CLASSES 3
#define TRIANGLE_TRAINING 26


//parameters of neural network
#define HIDDEN_1   40
#define HIDDEN_2   45
//#define ATTRIBUTES 23
using namespace std;
using namespace cv; 

void NeuralNetTrain(string path,char* Neural_output);
void loadPCA(const string &file_name,cv::PCA& pca_);
int Recognize(CvANN_MLP &nnetwork,PCA &pca,Mat test_img);

#endif
