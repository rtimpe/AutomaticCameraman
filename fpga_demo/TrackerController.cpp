#if 0 // COMMENTED_OUT_FOR_MAC_BUILD
#include "TrackerController.h"
#include "Tracker.h"
#include "FpgaTracker.h"

TS_INIT(trackercontroller, 4);
TC_INIT(trackercontroller, 2);

TrackerController *tc;
int ppos=0;
char fname[100];

////////////////////////////////////////////////////////////////////////////////
inline void update_fpga_params(TrackerController *tc, FpgaTrackerParams *fpgaParams) {
	int numTrackers = 0;
	int numFeats = 0;
	int numSelFeats = 0;
	MetaTracker *mt;
	for (map<int, MetaTracker *>::iterator it=tc->_metaTrackers.begin(); it != tc->_metaTrackers.end(); ++it) {
		mt = (*it).second;
		for (int j=0; j < mt->_numTrackers && numTrackers < MAX_NUM_TRACKERS; j++) {
			fpgaParams->_tr[numTrackers++] = mt->_tr[j];
			numFeats = max(numFeats, mt->_tr[j]->_clfparams->_numFeat);
			numSelFeats = max(numSelFeats, mt->_tr[j]->_clfparams->_numSel);
		}
	}
	fpgaParams->_numTrackers = numTrackers;
	fpgaParams->_numFeats = numFeats;
	fpgaParams->_numSelFeats = numSelFeats;
}


bool get_params(FpgaTrackerParams **fpgaParams) {
	MetaTracker *mt;
	FpgaTrackerParams *tmp;
	int numTrackers;
	int numFeats;
	int numSelFeats;
	IppiSize roi;

	// Release the previous frame and get the next image (wait if necessary)
	if ((tc->_lastFrameNum = tc->_framePool->release_acquire(&tc->_frame, tc->_lastFrameNum, true, true)) < 0)
		return false;
TS_STAMP(trackercontroller, 0);

	// Get our spinlock
	while (__sync_lock_test_and_set(&tc->_spinlock, 1) != 0);

	// Update the params if we have changes
	if (tc->_changedSet) {
printf("switching fpgaParams\n");
		tc->_changedSet = 0;
		tmp = tc->_fpgaParams;
		tc->_fpgaParams = tc->_fpgaParamsNext;
		tc->_fpgaParamsNext = tmp;
	}

	// Initialize if necessary
	if (tc->_fpgaParams->_grayIpl == NULL)
		tc->_fpgaParams->_grayIpl = cvCreateImage(cvSize(tc->_frame->_bgr->width, tc->_frame->_bgr->height), IPL_DEPTH_8U, 1);
	if (tc->_fpgaParamsNext->_grayIpl == NULL)
		tc->_fpgaParamsNext->_grayIpl = cvCreateImage(cvSize(tc->_frame->_bgr->width, tc->_frame->_bgr->height), IPL_DEPTH_8U, 1);

	// Convert to gray scale
	roi.width = tc->_frame->_bgr->width;
	roi.height = tc->_frame->_bgr->height;
	ippiRGBToGray_8u_C3C1R((Ipp8u*)tc->_frame->_bgr->imageData, tc->_frame->_bgr->width*3, 
		(Ipp8u*)tc->_fpgaParams->_grayIpl->imageData, tc->_fpgaParams->_grayIpl->width*1, roi);

	// Let the MetaTrackers have access to the images
	for (map<int, MetaTracker *>::iterator it=tc->_metaTrackers.begin(); it != tc->_metaTrackers.end(); ++it)
		((*it).second)->new_frame(tc->_frame);

	// Release our spinlock
	__sync_lock_release(&tc->_spinlock);

	// Set the return val
	*fpgaParams = tc->_fpgaParams;

TS_STAMP(trackercontroller, 1);
TC_ACCRUE(trackercontroller, 0, 1, 0);

	return true;
}


void adjust_location(FpgaTrackerParams *fpgaParams) {
	vector<Cluster *> preds;
	int maxCluster;
	float maxConfid;
	bool changed = false;

	IplImage *dispImg = cvCreateImage(cvSize(fpgaParams->_grayIpl->width, fpgaParams->_grayIpl->height), IPL_DEPTH_8U, 3);
	cvCvtColor(fpgaParams->_grayIpl, dispImg, CV_GRAY2BGR);

	// Let each MetaTracker update and return their x, y predictions. Cluster
	// confidence should be comparable across MetaTrackers
	preds.clear();
	for (map<int, MetaTracker *>::iterator it=tc->_metaTrackers.begin(); it != tc->_metaTrackers.end(); ++it)
		changed = changed || ((*it).second)->trackers_updated(&preds, tc->_attribs, fpgaParams->_grayIpl, dispImg);

	// Just select the cluster with the max score
	maxCluster = -1;
	maxConfid = -FLT_MAX;
	for (int i=0; i < preds.size(); ++i) {
		if (preds[i]->_confidence > maxConfid) {
			maxConfid = preds[i]->_confidence;
			maxCluster = i;
		}
	}

	// Use the cluster with the highest prediction
	if (maxCluster >= 0) {
//printf("clust: (%03d,%03d) %f\n", (int)preds[maxCluster]->_x, (int)preds[maxCluster]->_y, maxConfid);
		// Let each MetaTracker know about the new location
		for (map<int, MetaTracker *>::iterator it=tc->_metaTrackers.begin(); it != tc->_metaTrackers.end(); ++it)
			changed = changed || ((*it).second)->new_location(preds[maxCluster], fpgaParams->_grayIpl, dispImg);

		// Increment the tracked indicator (for anything that might care)
		pthread_mutex_lock(&tc->_mutex);
		tc->_tracked++;
		tc->_x = (int)preds[maxCluster]->_x;
		tc->_y = (int)preds[maxCluster]->_y;
		pthread_cond_signal(&tc->_cv_tracked);
		pthread_mutex_unlock(&tc->_mutex);
//printf("\n");
	}

	// Update the FpgaTrackerParams if changes have been made to the MetaTrackers
	if (changed) {
printf("updating fpgaParams\n");
		update_fpga_params(tc, fpgaParams);
	}

//cvNamedWindow("img", CV_WINDOW_AUTOSIZE);
//cvShowImage("img", dispImg);

//sprintf(fname, "./sylvester_anno/img_%05d.png", ppos++);
//cvSaveImage(fname, dispImg);

//cvWaitKey(2);
cvReleaseImage(&dispImg);
}


TrackerController::TrackerController(fpga_t *fpga, int chnl, long timeout, 
	FramePool *framePool) {
	_framePool = framePool;
	_attribs = new map<int, float>();
	_fpgaTracker = new FpgaTracker(fpga, chnl, timeout, get_params, adjust_location);
	_fpgaParams = new FpgaTrackerParams();
	_fpgaParamsNext = new FpgaTrackerParams();
	_x = 0;
	_y = 0;
	tc = this;
}


TrackerController::~TrackerController() {
	delete _attribs;
	delete _fpgaTracker;
	delete _fpgaParams;
	delete _fpgaParamsNext;
}


bool TrackerController::init() { 
	// Initialize locals
	_spinlock = 0;
	_changedSet = 0;
	_tracked = 0;
	_x = 0;
	_y = 0;
	_frame = NULL;
	_lastFrameNum = 0;
	_metaTrackers.clear();
	_fpgaParams->_numTrackers = 0;
	_fpgaParamsNext->_numTrackers = 0;
	_fpgaParams->_grayIpl = NULL;
	_fpgaParamsNext->_grayIpl = NULL;

	// Create the condition variables and mutex
	pthread_mutex_init(&_mutex, NULL);
	pthread_cond_init (&_cv_tracked, NULL);

	// Initialize the FpgaTracker
	if (!_fpgaTracker->init())
		return false;

	return true;
}


void TrackerController::start() {
	// Start the FpgaTracker
	_fpgaTracker->start();
}


void TrackerController::stop() {
	// Stop the FpgaTracker
	_fpgaTracker->stop();

	// Release any waiting threads
	pthread_mutex_lock(&_mutex);
	_tracked = -1;
	pthread_cond_signal(&_cv_tracked);
	pthread_mutex_unlock(&_mutex);

printf("trackercontroller: %f (%ld)\n", TC_ITER_AVG(trackercontroller, 0),  TC_ITERS(trackercontroller, 0));
}


void TrackerController::add(MetaTracker *mt) {
printf("updating\n");
	// Get our spinlock
	while (__sync_lock_test_and_set(&_spinlock, 1) != 0);

	// Update the metaTrackers
	_metaTrackers[mt->_id] = mt;

	// Update the next params 
	update_fpga_params(this, _fpgaParamsNext);
	_changedSet = 1;

	// Release our spinlock
	__sync_lock_release(&_spinlock);
}


void TrackerController::remove(MetaTracker *mt) {
	// Get our spinlock
	while (__sync_lock_test_and_set(&_spinlock, 1) != 0);

	// Update the metaTrackers
	_metaTrackers.erase(mt->_id);

	// Update the next params 
	update_fpga_params(this, _fpgaParamsNext);
	_changedSet = 1;

	// Release our spinlock
	__sync_lock_release(&_spinlock);
}


int TrackerController::tracked(int lastTracked, int *x, int *y, bool wait) {
	int rtnTracked;

	// Get our mutex
	pthread_mutex_lock(&_mutex);

	// Check if the latest is newer (different) than what we have
	while (wait && _tracked == lastTracked) 
		pthread_cond_wait(&_cv_tracked, &_mutex);

	// Update the response
	rtnTracked = _tracked;
	*x = _x;
	*y = _y;

	// Release our mutex
	pthread_mutex_unlock(&_mutex);

	// Return
	return rtnTracked;
}


#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
