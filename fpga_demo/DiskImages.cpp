#include "VideoImages.h"
#include "Matrix.h"

TS_INIT(diskimages, 4);
TC_INIT(diskimages, 2);


//////////////////////////////////////////////////////////////////////////////////////////////////////////

void *play_fxn(void *arg) {
	DiskImages *di = (DiskImages *)arg;
	Frame *frame = NULL;
	Matrixu mat;
	char path[100];

	for (int c = di->_startFrame; c <= di->_endFrame && di->_end == 0; c++) {
TS_STAMP(diskimages, 0);
		// Set the current frame, get next one for next time.
		frame = di->_pool->update_next(frame, c - di->_startFrame, true);

		// Load the RGB image file
		sprintf(path, di->_stem, di->_prefix, c);
		mat.LoadImage(path, 1);
		mat.createIpl();
		cvCopy(mat.getIpl(), frame->_bgr);
		mat.freeIpl();
TS_STAMP(diskimages, 1);
TC_ACCRUE(diskimages, 0, 1, 0);
	}

	return NULL;
}


DiskImages::DiskImages(char *prefix, char *stem, int startFrame, int endFrame, 
	int w, int h, int fps, FramePool *pool) : VideoImages(pool, w, h, fps) {
	// Save
	_startFrame = startFrame;
	_endFrame = endFrame;
	_prefix = prefix;
	_stem = stem;
}


bool DiskImages::init() {
	// Initialize locals
	_end = 0;

	// Initialize the FramePool
	for (int i = 0; i < _pool->_size; i++)
		_pool->add_frame(cvCreateImage(cvSize(_width, _height), IPL_DEPTH_8U, 3));

	return true;
}


void DiskImages::start() {
	// Create and start the threads
	pthread_create(&_thread, NULL, play_fxn, this);
}


void DiskImages::stop() {
	// Signal stop
	_end = 1;

	// Wait until the thread is done
    pthread_join(_thread, NULL);
printf("diskimages: %f\n", TC_ITER_AVG(diskimages, 0));
}


