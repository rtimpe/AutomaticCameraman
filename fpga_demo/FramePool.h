#ifndef FRAME_POOL_PUBLIC
#define FRAME_POOL_PUBLIC

#include "Public.h"

#define MAX_NUM_FRAMES 10

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class Frame
{
public:
	Frame(){};

	int _id;											// Unique id to identify IplImage 
	IplImage *_bgr;										// IplImage color
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class FramePool
{
public:
	FramePool(int size);
	~FramePool(){};
	void add_frame(IplImage *bgr);
	void destroy();
	Frame *update_next(Frame *frame, int frameNum, bool wait);
	int acquire(Frame **frame, int lastFrameNum, bool markAcquired, bool wait);
	void release(Frame *frame);
	int release_acquire(Frame **frame, int lastFrameNum, bool markAcquired, bool wait);

	int _size;											// Size of the pool
	int _numFrames;										// Number of Frames actually in the pool
	Frame *_frames[MAX_NUM_FRAMES];						// Pool
	int _refCounts[MAX_NUM_FRAMES];						// Reference counts for Frames
	bool _acquired[MAX_NUM_FRAMES];						// Acquired booleans for Frames
	pthread_mutex_t _mutex;								// Pool mutex
	pthread_cond_t _cv_curr;							// Acquire thread condition variable
	pthread_cond_t _cv_consumed;						// Consumed thread condition variable
	Frame *_currFrame;									// Current Frame
	int _currFrameNum;									// Current Frame number
	int _end;											// End signal
};


#endif

