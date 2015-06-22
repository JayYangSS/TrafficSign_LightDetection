#include "traffic.h"


//Helper function to find a cosine of angle between vectors from pt0->pt1 and pt0->pt2
static double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
	double dx1 = pt1.x - pt0.x;
	double dy1 = pt1.y - pt0.y;
	double dx2 = pt2.x - pt0.x;
	double dy2 = pt2.y - pt0.y;
	return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
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


Mat ShapeRecognize(Mat src,vector<Rect>&boudingBox)
{
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
				boudingBox.push_back(rect);
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
				if (vtc <= 6 && mincos >= -0.8 && maxcos <= -0.45)
				{
					setLabel(dst, "Octagon",rect);
					boudingBox.push_back(rect);
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
					boudingBox.push_back(rect);
				}
					
			}

	    }
		return dst;
	}
}