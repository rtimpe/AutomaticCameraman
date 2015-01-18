#ifndef META_TRACKER_PUBLIC
#define META_TRACKER_PUBLIC

#include "Public.h"
#include "FramePool.h"
#include "Annotator.h"
#include "FpgaTracker.h"
#include "Tracker.h"

#define MT_PEN_UP 1
#define MT_PEN_UP_SCORE 2

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class Cluster
{
public:
	int _num;											// Number in the cluster
	bool _stable;		
	float _x;											// x coordinate
	float _y;											// y coordinate
	float _confidence;									// Confidence value of the cluster
	float _dim;											// Dimension
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class MetaTracker : public Annotator
{
public:
	MetaTracker(): _numTrackers(0) {};
	~MetaTracker(){};
	virtual void new_frame(Frame *frame) = 0;
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg) = 0;
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) = 0;

	Tracker *_tr[MAX_NUM_TRACKERS];						// Trackers (_x,_y,_w,_h in resized coordinate space)
	int _numTrackers;									// Number of Trackers
};



#endif



