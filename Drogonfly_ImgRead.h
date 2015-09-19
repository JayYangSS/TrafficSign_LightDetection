#pragma once
/*#include "c:\program files\point grey research\flycapture2\include\busmanager.h"
#include "c:\program files\point grey research\flycapture2\include\camera.h"
#include "c:\program files\point grey research\flycapture2\include\error.h"*/
#include "FlyCapture2.h"
#include <iostream>
#include "opencv2/opencv.hpp"
/*#include "c:\program files\point grey research\flycapture2\include\flycapture2defs.h"
#include "c:\program files\point grey research\flycapture2\include\image.h"*/

using namespace FlyCapture2;
using namespace std;
using namespace cv;

class Drogonfly_ImgRead
{
public:
	Drogonfly_ImgRead(void);
	~Drogonfly_ImgRead(void);
	// Initial the Dragonfly Camera
	int Camera_Intial(void);
private:
	BusManager busMgr;
public:
	unsigned int numCameras;
	Camera cam;
	Error error;
	PGRGuid guid;
	// raw data captured from the dragonfly camera
	Image rawImage;
	IplImage* cvImg;
	// transfer the image data from camera to IplImage
	IplImage* Camera2IplImage(void);
	// clear the image data in the camera
	void ClearBuffer(void);
};

