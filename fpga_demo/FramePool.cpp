#include "FramePool.h"
#include "timer.h"
#include <assert.h>

TS_INIT(framepool, 4);
TC_INIT(framepool, 2);


FramePool::FramePool(int size) {
	_size = size;
	_currFrameNum = 0;
	_currFrame = NULL;
	_numFrames = 0;
	_end = 0;

	if (_size > MAX_NUM_FRAMES)
	{
	    printf("Error: Max number of frames for the Frame Pool is %d\n",
	           MAX_NUM_FRAMES);
	    exit(-1);
	}

	// Create the condition variables and mutex
	pthread_mutex_init(&_mutex, NULL);
	pthread_cond_init (&_cv_curr, NULL);
	pthread_cond_init (&_cv_consumed, NULL);
}


void FramePool::add_frame(IplImage *bgr) {
	// Populate the pool
	_refCounts[_numFrames] = 0;
	_frames[_numFrames] = new Frame();
	_frames[_numFrames]->_id = _numFrames;
	_frames[_numFrames]->_bgr = bgr;
	_numFrames++;
}


void FramePool::destroy() {
	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Set exit flag
	_end = 1;

	// Signal and release the mutex
	pthread_cond_signal(&_cv_curr);
	pthread_cond_signal(&_cv_consumed);
	pthread_mutex_unlock(&_mutex);
}


Frame *FramePool::update_next(Frame *frame, int frameNum, bool wait) {
	Frame *rtnFrame = NULL;

	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Set as the latest frame
	_currFrame = frame;
	_currFrameNum = frameNum;
	if (frame != NULL)
		_acquired[frame->_id] = false;
	pthread_cond_signal(&_cv_curr);

	// Wait until consumed?
	while (wait && _end == 0 && frame != NULL && !_acquired[frame->_id]) 
		pthread_cond_wait(&_cv_consumed, &_mutex);

	// Find an unused frame (not the latest one)
	for (int i=0; i < _numFrames; i++) {
		if (_refCounts[_frames[i]->_id] == 0 && 
			_frames[i]->_bgr != NULL && 
			_frames[i]->_bgr->nSize == sizeof(IplImage) &&
			(frame == NULL || _frames[i]->_id != frame->_id)) {
			rtnFrame = _frames[i];
			rtnFrame->_frame_num = frameNum;
			break;
		}
	}

	// Release the mutex
	pthread_mutex_unlock(&_mutex);

	assert(rtnFrame != NULL);
	return rtnFrame;
}


int FramePool::acquire(Frame **frame, int lastFrameNum, bool markAcquired, bool wait) {
	int rtnFrameNum;

	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Check if the latest is newer (different) than what we have
	while (wait && _end == 0 && _currFrameNum == lastFrameNum) 
		pthread_cond_wait(&_cv_curr, &_mutex);

	// Update the response
	rtnFrameNum = _currFrameNum;
	if (_currFrameNum != lastFrameNum) {
		*frame = _currFrame;
		_refCounts[_currFrame->_id]++;
		if (markAcquired) {
			_acquired[_currFrame->_id] = true;
			pthread_cond_signal(&_cv_consumed);
		}
	}

	// Exit if necessary
	if (_end != 0)
		rtnFrameNum = -1;

	// Release our mutex
	pthread_mutex_unlock(&_mutex);

	// Return
	return rtnFrameNum;
}


void FramePool::release(Frame *frame) {
	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Decrement the reference count
        if (frame && frame->_id <  MAX_NUM_FRAMES)
            _refCounts[frame->_id]--;

	// Release our mutex
	pthread_mutex_unlock(&_mutex);
}


int FramePool::release_acquire(Frame **frame, int lastFrameNum, bool markAcquired, bool wait) {
	int rtnFrameNum;

	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Decrement the reference count
	if  (*frame != NULL)
		_refCounts[(*frame)->_id]--;

	// Check if the latest is newer (different) than what we have
	while (wait && _end == 0 && _currFrameNum == lastFrameNum)
		pthread_cond_wait(&_cv_curr, &_mutex);

	// Update the response
	rtnFrameNum = _currFrameNum;
	if (_currFrameNum != lastFrameNum) {
		*frame = _currFrame;
		_refCounts[_currFrame->_id]++;
		if (markAcquired) {
			_acquired[_currFrame->_id] = true;
			pthread_cond_signal(&_cv_consumed);
		}
	}

	// Exit if necessary
	if (_end != 0)
		rtnFrameNum = -1;

	// Release our mutex
	pthread_mutex_unlock(&_mutex);

	// Return
	return rtnFrameNum;
}


