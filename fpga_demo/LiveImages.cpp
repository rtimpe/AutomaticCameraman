#include "VideoImages.h"
#include "timer.h"

TS_INIT(liveimages, 4);
TC_INIT(liveimages, 2);


//////////////////////////////////////////////////////////////////////////////////////////////////////////

void *capturer_fxn(void *arg) {
	LiveImages *li = (LiveImages *)arg;
	Frame *frame = NULL;
	int recvd;
	int expected = li->_width*li->_height*3/4;

	// Reset
	fpga_reset(li->_fpga);

	// Request continuous capture
	fpga_send(li->_fpga, li->_chnl, NULL, 0, 0, 1, li->_timeout);

	// Continue until the end
	for (int c = 0; li->_end == 0; c++) {
TS_STAMP(liveimages, 0);
		// Set the current frame, get next one for next time.
		frame = li->_pool->update_next(frame, c, false);

		// Receive the frame data
		do {
			recvd = fpga_recv(li->_fpga, li->_chnl, frame->_bgr->imageData, expected, li->_timeout);
			if (recvd != expected)
				printf("Dropped frame\n");
		} 
		while (recvd != expected);
TS_STAMP(liveimages, 1);
TC_ACCRUE(liveimages, 0, 1, 0);
	}

printf("liveimages: %f (%ld)\n", TC_ITER_AVG(liveimages, 0),  TC_ITERS(liveimages, 0));

	return NULL;
}


LiveImages::LiveImages(fpga_t *fpga, int chnl, long timeout, int w, int h, int fps,
	FramePool *pool) : VideoImages(pool, w, h, fps) {
	// Save
	_timeout = timeout;
	_fpga = fpga;
	_chnl = chnl;
}


bool LiveImages::init() {
	IplImage *bgr;
	char * frameBuf;

	// Initialize the FramePool
	for (int i = 0; i < _pool->_size; i++) {
		if ((frameBuf = (char *)malloc(_width*_height*3)) == NULL) {
			printf("ERROR: Cannot allocate image buffer space.\n");
			return false;
		}
		bgr = cvCreateImageHeader(cvSize(_width, _height), IPL_DEPTH_8U, 3);
		bgr->imageData = frameBuf;
		_pool->add_frame(bgr);
	}

	return true;
}


void LiveImages::start() {
	// Initialize the stop signal
	_end = 0;

	// Create and start the threads
	pthread_create(&_thread, NULL, capturer_fxn, this);
}


void LiveImages::stop() {
	// Signal stop
	_end = 1;

	// Wait until the thread is done
    pthread_join(_thread, NULL);
}



