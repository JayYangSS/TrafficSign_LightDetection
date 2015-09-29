#include "traffic.h"
#include "math_utils.h"
//Helper function to find a cosine of angle between vectors from pt0->pt1 and pt0->pt2
static double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

//识别nhls图像中矩形框内的颜色
int RecColorInBox(Mat img)
{
	int redCount=0;
	int blueCount=0;
	int yellowCount=0;
	assert(img.channels() == 1);
	for (int i = 0; i < img.rows; ++i)
	{
		const uchar *img_data = img.ptr<uchar> (i);
		for (int j = 0; j < img.cols; ++j)
		{
			uchar pixelVal=*img_data++;
			if(pixelVal==R_VALUE)
				redCount++;
			else if(pixelVal==Y_VALUE)
				yellowCount++;
			else if(pixelVal==B_VALUE)
				blueCount++;
			else
				continue;
		}
	}
	//find max value
	int finalVal=0;
	int tmpCount=0;
	if (redCount>=yellowCount)
	{
		tmpCount=redCount;
		finalVal=R_VALUE;
	}else{
		tmpCount=yellowCount;
		finalVal=Y_VALUE;
	}
	if (blueCount>tmpCount)
		finalVal=B_VALUE;
	return finalVal;
}

// Helper function to display text in the center of a contour
void setLabel(cv::Mat& im, const std::string label, Rect r)
{
	int fontface = cv::FONT_HERSHEY_SIMPLEX;
	double scale = 0.4;
	int thickness = 1;
	int baseline = 0;

	cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);

	cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
	cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
	cv::putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
}


Mat ShapeRecognize(Mat src,vector<ShapeRecResult>&shapeResult)
{
	ShapeRecResult tmp;
	if (src.channels()!=1)
	{
		cout<<"The input image for shape recognition must be binary"<<endl;
	}
	else
	{
		// Convert to binary image using Canny
		Mat edge;
		Canny(src,edge,0,50,5);

		//Find contours
		vector<vector<Point>> contours;
		findContours(edge.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		// The array for storing the approximation curve
		std::vector<cv::Point> approx;

		// We'll put the labels in this destination image
		cv::Mat dst = src.clone();

		for (int i = 0; i < contours.size(); i++)
		{
			
			Rect rect=boundingRect(contours[i]);
			//constraints of width-height ratio
			bool RatioConstraint=(abs(1-(float)(rect.width)/((float)(rect.height)))<0.2);
			if(!RatioConstraint)continue;
			// Approximate contour with accuracy proportional to the contour perimeter
			cv::approxPolyDP(cv::Mat(contours[i]), approx, cv::arcLength(cv::Mat(contours[i]), true) * 0.02, true);


			// Skip small or non-convex objects 
			if (std::fabs(cv::contourArea(contours[i])) < 100 || !cv::isContourConvex(approx))
				continue;

			if (approx.size() == 3)
			{
				setLabel(dst, "TRI", rect);    // Triangles
				tmp.box=rect;
				tmp.shape=TRIANGLE;//三角形形状为1
				Mat cutMat=src(rect);
				tmp.color=RecColorInBox(cutMat);
				shapeResult.push_back(tmp);
			}
				

			else if (approx.size() >= 4 && approx.size() <= 6)
			{
				// Number of vertices of polygonal curve
				int vtc = approx.size();

				// Get the cosines of all corners
				std::vector<double> cos;
				for (int j = 2; j < vtc+1; j++)
					cos.push_back(angle(approx[j%vtc], approx[j-2], approx[j-1]));

				// Sort ascending the cosine values
				std::sort(cos.begin(), cos.end());

				// Get the lowest and the highest cosine
				double mincos = cos.front();
				double maxcos = cos.back();

				// Use the degrees obtained above and the number of vertices
				// to determine the shape of the contour
				if (vtc == 4 && mincos >= -0.1 && maxcos <= 0.3)
				{
					setLabel(dst, "RECT", rect);
					tmp.box=rect;
					tmp.shape=RECTANGLE;//2表示矩形
					Mat cutMat=src(rect);
					tmp.color=RecColorInBox(cutMat);
					shapeResult.push_back(tmp);
				}
					
				if (vtc <= 6 && mincos >= -0.8 && maxcos <= -0.45)
				{
					setLabel(dst, "Octagon",rect);
					tmp.box=rect;
					tmp.shape=HEXA;//3表示六边形
					Mat cutMat=src(rect);
					tmp.color=RecColorInBox(cutMat);
					shapeResult.push_back(tmp);
				}
					
			}
			else
			{
				// Detect and label circles
				double area = cv::contourArea(contours[i]);
				cv::Rect r = cv::boundingRect(contours[i]);
				float radius = (float)(r.width)/ 2;

				if (std::abs(1 - ((double)r.width / r.height)) <= 0.2 &&
					std::abs(1 - (area / (CV_PI * std::pow(radius, 2)))) <= 0.2)
				{
					setLabel(dst, "CIR", rect);
					tmp.box=rect;
					tmp.shape=CIRCLE;//4表示矩形
					Mat cutMat=src(rect);
					tmp.color=RecColorInBox(cutMat);
					shapeResult.push_back(tmp);
				}
				/*vector<Vec3f> circles;
				Mat tmp=src(rect);
				HoughCircles( src, circles, CV_HOUGH_GRADIENT, 1, tmp.rows/8, 200, 100, 0, 0 );
				if (circles.size()>0)
				{
					setLabel(dst, "CIR", rect);
					boudingBox.push_back(rect);
				}*/
					
			}
		
	    }
		return dst;
	}
}