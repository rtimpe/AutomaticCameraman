#if 0 // COMMENTED_OUT_FOR_MAC_BUILD

#ifndef TRACKER_CONTROLLER_PUBLIC
#define TRACKER_CONTROLLER_PUBLIC

#include "Public.h"
#include "FramePool.h"
#include "FpgaTracker.h"
#include "MetaTracker.h"
#include <pthread.h>
#include <riffa.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class TrackerController
{
public:
	TrackerController(fpga_t *fpga, int chnl, long timeout, FramePool *framePool);
	~TrackerController();
	bool init();
	void start();
	void stop();
	void add(MetaTracker *mt);
	void remove(MetaTracker *mt);
	int tracked(int lastTracked, int *x, int *y, bool wait);

	int _spinlock;										// Spinlock to protect MetaTracker config
	int _changedSet;									// Indicator that the MetaTracker config has changed
	int _tracked;										// Counter for how many frames have been tracked
	int _lastFrameNum;									// Last frame number tracked
	Frame *_frame;										// Frame from the FramePool
	FramePool *_framePool;								// FramePool for new images to track
	FpgaTracker *_fpgaTracker;							// FpgaTracker to perform tracking
	FpgaTrackerParams *_fpgaParams;						// Parameters for FpgaTracker
	FpgaTrackerParams *_fpgaParamsNext;					// Parameters for FpgaTracker (next)
	map<int, MetaTracker *> _metaTrackers;				// Map of MetaTrackers used as config
	pthread_mutex_t _mutex;								// Mutex for condition variable
	pthread_cond_t _cv_tracked;							// Tracked variable condition variable
	int _x;												// Current location x coordinate
	int _y;												// Current location y coordinate
	map<int, float> *_attribs;							// TrackerController attributes
};



#endif



#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
