/*
 * math_utils.h
 *
 * Some common mathematical functions, like getting minimum and maximum of
 * three integers.
 *
 * TODO: add more function
 * TODO: think more generic
 */
#ifndef MATH_UTILS_H_
#define MATH_UTILS_H_
#include "traffic.h"

#define M_PI 3.1416
#define R_CONDITION (h < hue_max || h > hue_min) && s > sat_min
#define B_CONDITION (h < hue_max && h > hue_min) && s > sat_min
#define Y_CONDITION (h < hue_max && h > hue_min) && s > sat_min

// Got it from the original Matlab code.
// The values for the red colour.
//#define R_HUE_MAX 11
#define R_HUE_MAX 15
#define R_HUE_MIN 240
#define R_SAT_MIN 25

// The values for the blue colour.
#define B_HUE_MAX 163
#define B_HUE_MIN 134
#define B_SAT_MIN 39

// The values for the yellow colour.
#define Y_HUE_MAX 52
#define Y_HUE_MIN 18
#define Y_SAT_MIN 30

//define the four level for different color
#define R_VALUE 80
#define B_VALUE 160
#define Y_VALUE 255

/**
 * Return the greatest value of three passed arguments.
 *
 * Unsigned integer has been used, because RGB values cant be minus.
 */
unsigned int
get_maximum(unsigned int r, unsigned int g, unsigned int b);

/**
 * Return the lowest value of three passed arguments.
 *
 * Unsigned integer has been used, because RGB values cant be minus.
 */
unsigned int
get_minimum(unsigned int r, unsigned int g, unsigned int b);

float	retrieve_luminance(unsigned int r, unsigned int g, unsigned int b);
float	retrieve_saturation(unsigned int r, unsigned int g, unsigned int b);
float	retrieve_theta(unsigned int r, unsigned int g, unsigned int b);
float	retrieve_normalised_hue(unsigned int r, unsigned int g, unsigned int b);
Mat convert_rgb_to_ihls(Mat rgb_image);
Mat	convert_ihls_to_nhs(Mat ihls_image, int colour = 0, int hue_max = R_HUE_MAX,int hue_min = R_HUE_MIN, int sat_min = R_SAT_MIN);
Mat	convert_ihls_to_seg(Mat ihls_image, int hue_max = R_HUE_MAX,int hue_min = R_HUE_MIN, int sat_min = R_SAT_MIN);

#endif /* MATH_UTILS_H_ */
