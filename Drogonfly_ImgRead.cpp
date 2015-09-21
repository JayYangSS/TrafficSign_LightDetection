#include "Drogonfly_ImgRead.h"

Drogonfly_ImgRead::Drogonfly_ImgRead(void)
	: numCameras(0)
	, cvImg(NULL)
{
}


Drogonfly_ImgRead::~Drogonfly_ImgRead(void)
{
	cvReleaseImage(&cvImg);
}


// Initial the Dragonfly Camera
int Drogonfly_ImgRead::Camera_Intial(void)
{
	Image convertedImage;
	//detect the num of cameras
	error=busMgr.GetNumOfCameras(&numCameras);
	if (error != PGRERROR_OK)
	{
		//PrintError( error );
		error.PrintErrorTrace();
		return -1;
	}
	cout<<"Number of cameras detected: " << numCameras << endl;

	//get the index of camera
	error=busMgr.GetCameraFromIndex(0,&guid);
	if (error!=PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	// Connect to a camera
	error = cam.Connect(&guid);
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	// Start capturing images
	error = cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	// Retrieve an image
	error = cam.RetrieveBuffer( &rawImage );
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	//convert the raw image to the format of BGR
	error=rawImage.Convert(PIXEL_FORMAT_BGR,&convertedImage);
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	//copy the data to IplImage
	cvImg=cvCreateImage(cvSize(rawImage.GetCols(), rawImage.GetRows()), 8, 3);
	cout<<"Dragonfly Initial Done!"<<endl;
}


// transfer the image data from camera to IplImage
IplImage* Drogonfly_ImgRead::Camera2IplImage(void)
{
	Image convertedImage;
	error= cam.RetrieveBuffer(&rawImage);
	if (error!=PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return NULL;
	}
	error = rawImage.Convert( FlyCapture2::PIXEL_FORMAT_BGR, &convertedImage);
	if (error !=PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return NULL;
	}
	memcpy(cvImg->imageData, convertedImage.GetData(), convertedImage.GetDataSize());
	if( !cvImg ) 
	{
		cout<<"No data in IplImage!"<<endl;
		return NULL;
	}
	return cvImg;
}


// clear the image data in the camera
void Drogonfly_ImgRead::ClearBuffer(void)
{
	error=cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
	} 
	error=cam.Disconnect();
	if (error!= PGRERROR_OK)
	{
		error.PrintErrorTrace();
	}
}
