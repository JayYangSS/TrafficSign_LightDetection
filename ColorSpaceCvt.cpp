#include "traffic.h"
#include "math_utils.h"

float	retrieve_luminance(unsigned int r, unsigned int g, unsigned int b)
{
	return (0.210f * r) + (0.715f * g) + (0.072f * b);
}


float	retrieve_saturation(unsigned int r, unsigned int g, unsigned int b)
{
	float saturation;
	unsigned int max = get_maximum(r, g, b);
	unsigned int min = get_minimum(r, g, b);

	saturation = max - min;

	return saturation;
}



float	retrieve_theta(unsigned int r, unsigned int g, unsigned int b)
{
	float theta;

	// The numerator part of equation
	float numerator = r - (g * 0.5) - (b * 0.5);

	// The denominator part of equation
	float denominator = (r * r) + (g * g) + (b * b) - (r * g) - (r * b) - (g * b);

	float temp = numerator / sqrtf(denominator);
	theta = acos(temp);

	return theta;
}


float	retrieve_normalised_hue(unsigned int r, unsigned int g, unsigned int b)
{
	float hue;
	if (b <= g)
	{
		hue = retrieve_theta(r, g, b);
	}
	else
	{
		hue = (2 * M_PI) - retrieve_theta(r, g, b);
	}

	return hue * 255 / (2 * M_PI);
}


Mat convert_rgb_to_ihls(Mat rgb_image)
{
	
	
	assert(rgb_image.channels() == 3);

	Mat ihls_image(rgb_image.rows, rgb_image.cols, CV_8UC3);

	for (int i = 0; i < rgb_image.rows; ++i)
	{
		const uchar* rgb_data = rgb_image.ptr<uchar> (i);
		uchar* ihls_data = ihls_image.ptr<uchar> (i);

		for (int j = 0; j < rgb_image.cols; ++j)
		{
			unsigned int b = *rgb_data++;
			unsigned int g = *rgb_data++;
			unsigned int r = *rgb_data++;
			*ihls_data++ = (uchar) retrieve_saturation(r, g, b);
			*ihls_data++ = (uchar) retrieve_luminance(r, g, b);
			*ihls_data++ = (uchar) retrieve_normalised_hue(r, g, b);
		}
	}

	return ihls_image;
}



Mat	convert_ihls_to_nhs(Mat ihls_image, int colour, int hue_max, int hue_min,int sat_min)
{
	
	//static int maxH=0;
	//static int minH=255,minS=255;


	if (colour == 2)//RED
	{
		if (hue_max > 255 || hue_max < 0 || hue_min > 255 || hue_min < 0
			|| sat_min > 255 || sat_min < 0)
		{
			hue_min = R_HUE_MIN;
			hue_max = R_HUE_MAX;
			sat_min = R_SAT_MIN;
		}
	}
	else if (colour == 1)//BLUE
	{
		hue_min = B_HUE_MIN;
		hue_max = B_HUE_MAX;
		sat_min = B_SAT_MIN;
	}
	else if(colour==0)//YELLOW
	{
		hue_min = Y_HUE_MIN;
		hue_max = Y_HUE_MAX;
		sat_min = Y_SAT_MIN;
	}

	else
	{
		hue_min = R_HUE_MIN;
		hue_max = R_HUE_MAX;
		sat_min = R_SAT_MIN;
	}

	assert(ihls_image.channels() == 3);

	Mat nhs_image(ihls_image.rows, ihls_image.cols, CV_8UC1);

	// I put the if before for loops, to make the process faster.
	// Otherwise for each pixel it had to check this condition.
	// Nicer implementation could be to separate these two for loops in
	// two different functions, one for red and one for blue.
	if (colour == 1)//blue
	{
		for (int i = 0; i < ihls_image.rows; ++i)
		{
			const uchar *ihls_data = ihls_image.ptr<uchar> (i);
			uchar *nhs_data = nhs_image.ptr<uchar> (i);
			for (int j = 0; j < ihls_image.cols; ++j)
			{
				uchar s = *ihls_data++;
				// Although l is not being used and we could have
				// replaced the next line with ihls_data++
				// but for the sake of readability, we left it as it it.
				uchar l = *ihls_data++;
				uchar h = *ihls_data++;

				*nhs_data++ = (B_CONDITION) ? 255 : 0;
			}
		}
	}
	else if (colour==0)//yellow
	{
		for (int i = 0; i < ihls_image.rows; ++i)
		{
			const uchar *ihls_data = ihls_image.ptr<uchar> (i);
			uchar *nhs_data = nhs_image.ptr<uchar> (i);
			for (int j = 0; j < ihls_image.cols; ++j)
			{
				uchar s = *ihls_data++;
				uchar l = *ihls_data++;
				uchar h = *ihls_data++;
				*nhs_data++ = (Y_CONDITION) ? 255 : 0;

			/*	if(maxH<h)maxH=h;
				if(minH>h)minH=h;
				if(minS>s)minS=s;*/

			}
		}
	}
	else
	{
		for (int i = 0; i < ihls_image.rows; ++i)
		{
			const uchar *ihls_data = ihls_image.ptr<uchar> (i);
			uchar *nhs_data = nhs_image.ptr<uchar> (i);
			for (int j = 0; j < ihls_image.cols; ++j)
			{
				uchar s = *ihls_data++;
				// Although l is not being used and we could have
				// replaced the next line with ihls_data++
				// but for the sake of readability, we left it as it it.
				uchar l = *ihls_data++;
				uchar h = *ihls_data++;
				*nhs_data++ = (R_CONDITION) ? 255 : 0;
			}
		}
	}

/*	cout<<"maxH:"<<maxH<<endl;
	cout<<"minH:"<<minH<<endl;
	cout<<"minS:"<<minS<<endl;*/

	return nhs_image;
}


Mat	convert_ihls_to_seg(Mat ihls_image, int hue_max, int hue_min,int sat_min)
{
	
	//static int maxH=0;
	//static int minH=255,minS=255;
	assert(ihls_image.channels() == 3);

	Mat nhs_image(ihls_image.rows, ihls_image.cols, CV_8UC1);

	// I put the if before for loops, to make the process faster.
	// Otherwise for each pixel it had to check this condition.
	// Nicer implementation could be to separate these two for loops in
	// two different functions, one for red and one for blue.
		for (int i = 0; i < ihls_image.rows; ++i)
		{
			const uchar *ihls_data = ihls_image.ptr<uchar> (i);
			uchar *nhs_data = nhs_image.ptr<uchar> (i);
			for (int j = 0; j < ihls_image.cols; ++j)
			{
				uchar s = *ihls_data++;
				uchar l = *ihls_data++;
				uchar h = *ihls_data++;

				if ((h < B_HUE_MAX && h > B_HUE_MIN) && s > B_SAT_MIN)
					*nhs_data++=B_VALUE;
				else if( (h < R_HUE_MAX || h > R_HUE_MIN) && s > R_SAT_MIN)
					*nhs_data++=R_VALUE;
				else if((h < Y_HUE_MAX && h > Y_HUE_MIN) && s > Y_SAT_MIN)
					*nhs_data++=Y_VALUE;
				else
					*nhs_data++=0;
			}
		}

	return nhs_image;
}