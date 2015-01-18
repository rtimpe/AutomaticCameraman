#ifndef LIBRARY_TRACKER_PUBLIC
#define LIBRARY_TRACKER_PUBLIC

#include "Public.h"
#include "BaseTracker.h"
// 18
#define TRAIN_THRESH 5
#define NCC_DIST 7

//////////////////////////////////////////////////////////////////////////////////////////////////////////

class LibraryTracker : public BaseTracker
{
public:
	LibraryTracker(int id);
	bool init(int centerX, int centerY, int dim, int numFeats, int numSelFeats, IplImage *gray);
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg);
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg);

	Cluster _clusters[MAX_NUM_TRACKERS];				// Cluster
	int _trainCount;									// Count frames for training
	int _detectionCount;								// Count frames for detections
};



#endif



