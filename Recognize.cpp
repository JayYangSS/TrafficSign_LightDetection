#include "HOG_ANN.h"


void covertImg2HOG(Mat &img, Mat &img_vect)
{
	vector<float> descriptors;
	HOGDescriptor hog(Size(40,40),Size(10,10),Size(5,5),Size(5,5),9,1,-1.0,0,0.2,true,64);
	hog.compute(img,descriptors,Size(8,8));
	//cout<<"HOGÌØÕ÷×ÓÎ¬Êý£º"<<descriptors.size()<<endl;


	int FeatureLength=descriptors.size();
	for (int i=0;i<FeatureLength;i++)
	{
		img_vect.at<double>(0,i)=descriptors[i];
	}

}


double predict(Mat & sample,CvANN_MLP& nnetwork,int numClasses)
{
	
			cv::Mat classificationResult(1, numClasses, CV_64F);
            nnetwork.predict(sample, classificationResult);
            /*The classification result matrix holds weightage  of each class. 
            we take the class with the highest weightage as the resultant class */
 
            // find the class with maximum weightage.
            int maxIndex = 0;
            double value=0.0;
            double maxValue=classificationResult.at<double>(0,0);
            for(int index=1;index<numClasses;index++)
            {   value = classificationResult.at<double>(0,index);
                if(value>maxValue)
                {   maxValue = value;
                    maxIndex=index;
 
                }
            }

       
return maxIndex + 1;

} 


int Recognize(CvANN_MLP &nnetwork,PCA &pca,Mat test_img,int numClasses)
{

			string img_path;
			Mat test_img_vect(1,RESIZED_IMG_DIM,CV_64FC1);
			// resizing img to the standard size of 40*40*3
			Mat resizedImg(IMG_NEW_DIM,IMG_NEW_DIM,CV_8UC3) ;
			resize(test_img , resizedImg ,  resizedImg.size() );
			// convert the image 3D matrix to a vector
			covertImg2HOG( resizedImg, test_img_vect);
			// apply PCA to that image to reduce it dimension
			Mat test_img_Reduce = pca.project(test_img_vect);

			int pred;//predict result
			pred = predict(test_img_Reduce, nnetwork,numClasses);
			//cout << " predicted value :   " << pred<< endl << endl << endl;

			return pred;
}