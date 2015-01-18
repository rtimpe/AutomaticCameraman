#ifndef VIDEO_IMAGES_PUBLIC
#define VIDEO_IMAGES_PUBLIC

#include "Public.h"
#include "FramePool.h"
#include <pthread.h>
#include <riffa.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class VideoImages
{
public:
	VideoImages(FramePool *pool, int w, int h, int fps) { _pool = pool; _width = w; _height = h; _fps = fps; };
	virtual bool init() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;

	FramePool *_pool;									// Frame pool
	int _width;											// Width of live images
	int _height;										// Height of live images
	int _fps;											// Target frames per second
};


class LiveImages : public VideoImages
{
public:
	LiveImages(fpga_t *fpga, int chnl, long timeout, int w, int h, int fps, FramePool *pool);
	virtual bool init();
	virtual void start();
	virtual void stop();

	fpga_t *_fpga;										// FPGA handle
	int _chnl;											// FPGA channel
	long _timeout;										// FPGA communication timeout (ms)
	pthread_t _thread;									// Capture thread
	int _end;											// End signal
};


class DiskImages : public VideoImages
{
public:
	DiskImages(char *prefix, char *stem, int startFrame, int endFrame, int w, 
		int h, int fps, FramePool *pool);
	virtual bool init();
	virtual void start();
	virtual void stop();

	int _startFrame;									// Start frame of disk images
	int _endFrame;										// End frame of disk images
	char *_prefix;										// String prefix of disk images path
	char *_stem;										// String stem format for disk images
	int _end;											// End signal
	pthread_t _thread;									// Thread
};


#endif

