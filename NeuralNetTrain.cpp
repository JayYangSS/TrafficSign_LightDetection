#include "HOG_ANN.h"

#define TEST_SAMPLES 10
#define  DATA_SET_SIZE 22
cv::Mat HOGFeature;
//#define PCA_FILE "pcaTriangle.yml"


void read_dataset(string filename, cv::Mat &data, cv::Mat &labels)
{

	Mat dataSet, classes;


	FileStorage fs(filename,FileStorage::READ);

	replace(filename.begin(),filename.end(),'.',' ');
	stringstream iss(filename);
	string readfileName;
	iss>>readfileName;
	fs[readfileName] >> dataSet ; fs.release();

	// randomly shuffle the matrix rows
	//MatShuffle(dataSet);



	data = dataSet( Range::all(),Range(0,RESIZED_IMG_DIM) ).clone();
	//labs = dataSet.col(RESIZED_IMG_DIM).clone();
	classes=dataSet.col(RESIZED_IMG_DIM).clone();

	// construct the a label vector of size 13 for each image 
	// each of this vector has a unique cell sets to 1 (others=0) with it index corresponding to the image ClassId

	for(int i=0 ; i <  classes.rows ; i++)	
	{
		
			switch( (int)classes.at<double>(i,0))
			{
			case 1 : labels.at<double>(i,0) = 1; break;
			case 2 : labels.at<double>(i,1) = 1; break;
			case 3 : labels.at<double>(i,2) = 1; break;
			default: break;
			}
	}
}


void loadPCA(const string &file_name,cv::PCA& pca_)
{
	FileStorage fs(file_name,FileStorage::READ);
	fs["mean"] >> pca_.mean ;
	fs["e_vectors"] >> pca_.eigenvectors ;
	fs["e_values"] >> pca_.eigenvalues ;
	fs.release();

}



void NeuralNetTrain(string shufflePath,string Neural_output,PCA &pca,int trainingSampleNum,int numClasses)
{

	cv::Mat labels = Mat::zeros(trainingSampleNum, numClasses, CV_64F);
	int n = 0; 


	//matrix to hold the training samples
	cv::Mat training_set;
	//matrix to hold the training labels.
	cv::Mat training_set_classifications(trainingSampleNum,numClasses,CV_64F);




	//load the training and test data sets.
	read_dataset(shufflePath, HOGFeature, labels);


	// load the pca cov_matrix	
	//PCA pca;
	//loadPCA( pcaPath, pca);


	// reduce the dimension of the data set to 388 (99% of the variance retained)
	Mat datasetReduce = pca.project(HOGFeature);
	HOGFeature.release();


	// split the dataSet into training and test sets
	//training_set = dataset388(Range(0,TRIANGLE_TRAINING) , Range::all() ).clone();
	training_set = datasetReduce(Range(0,trainingSampleNum) , Range::all() ).clone();
	//test_set = dataset388(Range(TRAINING_SAMPLES , DATA_SET_SIZE ) ,  Range::all() ).clone();
	datasetReduce.release();

	// do the same with the assiocated labels
	training_set_classifications = labels(Range(0,trainingSampleNum) , Range::all() ).clone();
	//test_set_classifications = labels(Range(TRAINING_SAMPLES , DATA_SET_SIZE ) ,  Range::all() ).clone();
	labels.release();




	// define the structure for the neural network 

	int inputlayer=training_set.cols;
	cv::Mat layers(3,1,CV_32S);
	layers.at<int>(0,0) = inputlayer;//input layer
	layers.at<int>(1,0)= HIDDEN_1;//hidden1 layer
	layers.at<int>(2,0) =numClasses;//output layer

	//create the neural network.
	//for more details check http://docs.opencv.org/modules/ml/doc/neural_networks.html
	CvANN_MLP nnetwork(layers, CvANN_MLP::SIGMOID_SYM,0.6,1);

	CvANN_MLP_TrainParams params(                                   

		// terminate the training after either 1000 
		// iterations or a very small change in the
		// network wieghts below the specified value
		cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 2000, 0.00001),
		// use backpropogation for training
		CvANN_MLP_TrainParams::BACKPROP, 
		// co-efficents for backpropogation training
		// recommended values taken from http://docs.opencv.org/modules/ml/doc/neural_networks.html#cvann-mlp-trainparams
		0.0001, 
		0.1);

	// train the neural network (using training data)

	printf( "\nUsing training dataset\n");

	int iterations = nnetwork.train(training_set, training_set_classifications,cv::Mat(),cv::Mat(),params);
	printf( "Training iterations: %i\n\n", iterations);

	// Save the model generated into an xml file.
	CvFileStorage* storage = cvOpenFileStorage(Neural_output.c_str(), 0, CV_STORAGE_WRITE );

	replace(Neural_output.begin(),Neural_output.end(),'.',' ');
	stringstream iss(Neural_output);
	string Neural_outputName;
	iss>>Neural_outputName;


	nnetwork.write(storage,Neural_outputName.c_str());
	cvReleaseFileStorage(&storage); 



}
