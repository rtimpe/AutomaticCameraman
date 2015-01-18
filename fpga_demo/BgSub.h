#ifndef BACKGROUND_SUBTRACTION_PUBLIC
#define BACKGROUND_SUBTRACTION_PUBLIC

#include "Public.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define IMG_WIDTH 1280
#define IMG_HEIGHT 720
#define NUM_SAMPLES 20

class BgSub
{
public:
	BgSub(int w, int h, int fps);
	~BgSub(){};
	bool init();
	void get_center_of_mass(const int x, const int y, const int w, const int h, 
		float *xAvg, float *yAvg, float *fillPercent);
	void update_mask(IplImage *bgr, int x, int y, int w, int h, float thresh);
	void start();
	void stop();

	unsigned int _shortSamples[3][IMG_WIDTH][IMG_HEIGHT][NUM_SAMPLES];	// Samples array for short term history
	unsigned int _longSamples[3][IMG_WIDTH][IMG_HEIGHT][NUM_SAMPLES];	// Samples array for long term history
	IplImage *_maskIpl;									// Background subtracted image
	int _count;											// Count of mask updates
	int _shortUpdate;									// Update frequency for short term history
	int _longUpdate;									// Update frequency for long term history
	int _width;											// Size of images (width)
	int _height;										// Size of images (height)
	int _end;											// Thread termination signal
	int _update;										// Mask update request signal
	int _updateX;										// Mask update x coordinate
	int _updateY;										// Mask update y coordinate
	int _updateW;										// Mask update width
	int _updateH;										// Mask update height
	float _updateThresh;								// Mask update threshold
	IplImage *_updateImg;								// Mask update IplImage
	pthread_mutex_t _mutex;								// Mutex for condition variable
	pthread_cond_t _cv_update;							// Update variable condition variable
	pthread_cond_t _cv_updated;							// Updated variable condition variable
	pthread_t _thread;									// Update thread
};


#endif



