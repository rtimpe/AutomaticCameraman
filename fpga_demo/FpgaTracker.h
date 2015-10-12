#if 0 // COMMENTED_OUT_FOR_MAC_BUILD
#ifndef FPGA_TRACKER_PUBLIC
#define FPGA_TRACKER_PUBLIC

#include "Public.h"
#include "Tracker.h"
#include <riffa.h>

#define MIN_SEARCH_RADIUS 11
#define MAX_SEARCH_RADIUS 40
#define MAX_NUM_FEATS 250
#define MAX_NUM_SEL_FEATS 50
#define MAX_NUM_TRACKERS 20

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

class FpgaTrackerParams
{
public:
	FpgaTrackerParams(){ _numTrackers = 0; };

	Tracker *_tr[MAX_NUM_TRACKERS];						// Trackers (_x,_y,_w,_h in resized coordinate space)
	int _numTrackers;									// Number of Trackers
	int _numFeats;										// Number of total features (same for all Trackers)
	int _numSelFeats;									// Number of selected (classifier) features (same for all Trackers)
	IplImage *_grayIpl;									// Grayscale image - after histogram normailization
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

class FpgaTracker
{
public:
	FpgaTracker(fpga_t *fpga, int chnl, long timeout,
		bool (*getParamsFxn)(FpgaTrackerParams **), 
		void (*adjustLocFxn)(FpgaTrackerParams *)); 
	~FpgaTracker(){};
	bool init();
	void start();
	void stop();

	FpgaTrackerParams *_fpgaParams;						// Current FpgaTrackerParams
	pthread_t _threadSend; 								// Sender thread
	pthread_t _threadRecv; 								// Receiver thread
	pthread_mutex_t _mutex;								// Thread mutex
	pthread_cond_t _cv_send;							// Sender thread condition variable
	pthread_cond_t _cv_recv;							// Receiver thread condition variable
	bool (*_getParamsFxn)(FpgaTrackerParams **);		// Function pointer to get next FpgaTrackerParams
	void (*_adjustLocFxn)(FpgaTrackerParams *);			// Function pointer to adjust found location(s)
	fpga_t *_fpga;										// FPGA handle
	int _chnl;											// FPGA channel to communicate over
	long _timeoutMs;									// FPGA communications timeout (in ms)
	unsigned int *_sendBuf; 							// Buffer location for sending data to the FPGA
	unsigned int *_recvBuf;								// Buffer location for receiving data from the FPGA
	int _start;											// Synchronization between threads
	int _end;											// Flag to signal threads should exit
};



#endif


#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
