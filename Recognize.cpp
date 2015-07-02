#include "HOG_ANN.h"


void covertImg2HOG(Mat &img, Mat &img_vect)
{
	vector<float> descriptors;
	HOGDescriptor hog(Size(40,40),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,64);
	hog.compute(img,descriptors,Size(8,8));
	cout<<"HOGÌØÕ÷×ÓÎ¬Êý£º"<<descriptors.size()<<endl;


	int FeatureLength=descriptors.size();
	for (int i=0;i<FeatureLength;i++)
	{
		img_vect.at<double>(0,i)=descriptors[i];
	}

}


double predict(Mat & sample,CvANN_MLP& nnetwork)
{
	
			cv::Mat classificationResult(1, TRIANGLE_CLASSES, CV_64F);
            nnetwork.predict(sample, classificationResult);
            /*The classification result matrix holds weightage  of each class. 
            we take the class with the highest weightage as the resultant class */
 
            // find the class with maximum weightage.
            int maxIndex = 0;
            double value=0.0;
            double maxValue=classificationResult.at<double>(0,0);
            for(int index=1;index<TRIANGLE_CLASSES;index++)
            {   value = classificationResult.at<double>(0,index);
                if(value>maxValue)
                {   maxValue = value;
                    maxIndex=index;
 
                }
            }

       
return maxIndex + 1;

} 


int Recognize(CvANN_MLP &nnetwork,PCA &pca,Mat test_img)
{
	//PCA pca;
	
	//loadPCA(pcaPath, pca);

	string img_path;

//	cout <<  "path to road sign image to recognize : ";
//	cin >> img_path;


	//while(img_path.c_str())
//	{
			// get the image to recognize
		//	Mat test_img =  imread(img_path);
			imshow("img",test_img); 
			waitKey(2);


			Mat test_img_vect(1,RESIZED_IMG_DIM,CV_64FC1);

			// resizing img to the standard size of 40*40*3
			Mat resizedImg(IMG_NEW_DIM,IMG_NEW_DIM,CV_8UC3) ;
			resize(test_img , resizedImg ,  resizedImg.size() );

			// convert the image 3D matrix to a vector
			covertImg2HOG( resizedImg, test_img_vect);

			// apply PCA to that image to reduce it dimension ( 4800 ---> 388 )
			Mat test_img_Reduce = pca.project(test_img_vect);


 
			// define the structure for the neural network (MLP)
			// The neural network has 3 layers.
			// - one input node per attribute in a sample so 388 input nodes
			// - 500 hidden nodes
			// - 13 output node, one for each class.
		/*    int inputlayer=test_img_Reduce.cols;
 
			cv::Mat layers(3,1,CV_32S);
			layers.at<int>(0,0) = inputlayer;//input layer
			layers.at<int>(1,0)= HIDDEN_1;//hidden layer
			layers.at<int>(2,0) =TRIANGLE_CLASSES;//output layer
 
			//create the neural network.
			//for more details check http://docs.opencv.org/modules/ml/doc/neural_networks.html
			CvANN_MLP nnetwork(layers, CvANN_MLP::SIGMOID_SYM,1,1);*/
			//CvANN_MLP nnetwork;

			// retrieve the neural network modeled previously
			//nnetwork.load(xmlPath, "xmlTriangle");
			int pred;

			clock_t start=clock();
 
			//pred = predict(test_img_388, nnetwork);
			pred = predict(test_img_Reduce, nnetwork);
			clock_t end=clock();
			cout<<"Run time: "<<(double)(end - start) / CLOCKS_PER_SEC<<"s"<<endl;
			cout << " predicted value :   " << pred<< endl << endl << endl;
	
	
		//	cout <<  " path to a new image: " ;
		//	cin >>  img_path ;
			return pred;
 
//	}     
}