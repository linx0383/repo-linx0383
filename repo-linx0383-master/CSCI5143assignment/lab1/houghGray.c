#include <math.h>
#include "houghGray.h"

#include "image40.h"
double PI;

#define RRATIO 1.5
#define TRATIO 1.5
#define GAMMA 1.5

const uint8_t rhoSize = (uint8_t) (__Height / RRATIO );
const uint8_t thetaSize = (uint8_t) (__Width / TRATIO );

void houghTransform( uint16_t rPtr, uint16_t gPtr, uint16_t bPtr )
{
	int i, j, k;
	int m, l ;
	double bg;
	double x,y,rr,r_res,t_res;
	double max_val, min_val, valRange;
	double rho, theta, v ;

	double transformImage[rhoSize][thetaSize];
	double intensityImage[__Height][__Width];

	uint16_t pixPtr;
	uint8_t rValue, gValue, bValue;

	// Transform RGB to a single intenstity image (matrix)
	for (i = 0; i < __Height; i++) {
		for (j = 0; j < __Width; j++) {
			pixPtr = rPtr + (i*j + j);
			rValue = pgm_read_byte_near(pixPtr);
			pixPtr = gPtr + (i*j + j);
			gValue = pgm_read_byte_near(pixPtr);
			pixPtr =	bPtr + (i*j + j);
			bValue = pgm_read_byte_near(pixPtr);
			intensityImage[i][j] = (rValue * 0.2126 + bValue * 0.7152 + gValue * 0.0722) / 255;
		}
	}

	PI = atan2(1, 1) * 4;

	/* first, run through all pixels and see what the average is,
	* so we can take a guess if the background is black or white.
	* a real application wouldn't do silly things like this	*/
	for (i = 0, bg = 0; i < __Height; i++) {
		for (j = 0; j < __Width; j++) {
			bg += intensityImage[i][j];
		}
	}
	bg = (bg /= (__Height * __Width) > 0.5) ? 1 : 0;

	/* if white, invert it */
	if (bg) {
		for (i = 0; i < __Height; i++)
			for (j = 0; j < __Width; j++)
				intensityImage[i][j] = 1 - intensityImage[i][j];
	}

	// TODO: zero contents of transformImage

	/* second, decide what resolution of rho and theta should be.
	* here we just make the rho/theta graph a fixed ratio
	* of input, which is dumb.  It should depend on the application.
	* finer bins allow better resolution between lines, but will
	* lose contrast if the input is noisy.  Also, lower resolution, faster.
	*/
	x = __Width - 0.5;
	y = __Height - 0.5;
	rr = sqrt(x * x + y * y) / 2.0 ;
	r_res = rr / rhoSize;
	t_res = PI * 2.0 / thetaSize;

	for (i = 0; i < __Height; i++) {
		y = i - __Height / 2. + .5;
		for (j = 0; j < __Width; j++) {
			x = j - __Width / 2 + .5;
			rr = sqrt(x * x + y * y);
			v = intensityImage[i][j];

			/* at each pixel, check what lines it could be on */
			for (k = 0; k < thetaSize; k++) {
				theta = k * t_res - PI;
				rho = x * cos(theta) + y * sin(theta);
				if (rho >= 0) {
					m = (int) rho / r_res;
					l = k;
				}
				else {
					m = (int) ( -rho / r_res );
					l = (int) ( k + thetaSize/2.0 );
					l %= thetaSize ;
				}
				transformImage[m][l] += (double) (v * rr);
			}
		}
		/* show which row we are precessing lest user gets bored */
		// TO FIX: For debuggin, print out something to indicate progress
	}

	max_val = transformImage[0][0];
	min_val = transformImage[0][0];

	for (i = 0; i < rhoSize; i++) {
		for (j = 0; j < thetaSize; j++) {
			if (max_val < transformImage[i][j]) max_val = transformImage[i][j];
			if (min_val > transformImage[i][j]) min_val = transformImage[i][j];
		}
	}

	/* gamma correction. if gamma > 1, output contrast is better, noise
	is suppressed, but spots for thin lines may be lost; if gamma < 1,
	everything is brighter, both lines and noises */
	valRange = max_val - min_val;
	for (i = 0; i < rhoSize; i++) {
		for (j = 0; j < thetaSize; j++) {
			transformImage[i][j] = pow((transformImage[i][j] - min_val) / valRange, GAMMA );
		}
	}
	// Transform is complete. Nothing to do with it, so we will return and lose all our work.
}
