#if 0 // COMMENTED_OUT_FOR_MAC_BUILD
#ifndef BASE_TRACKER_PUBLIC
#define BASE_TRACKER_PUBLIC

#include "Public.h"
#include "MetaTracker.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class BaseTracker : public MetaTracker
{
public:
	BaseTracker(int id);
	~BaseTracker();
	bool init(int centerX, int centerY, int dim, int numFeats, int numSelFeats, IplImage *gray);
	bool init_trackers(Tracker *tr[], int centerX[], int centerY[], int dim[], 
		float lRate[], int numTrackers, int numFeats, int numSelFeats, IplImage *gray);
	virtual bool draw_long_term(IplImage *img) { return false; };
	virtual void draw_short_term(IplImage *img);
	virtual void new_frame(Frame *frame) {};
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg);
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg);

	Cluster _cluster;									// Cluster
};



#endif


#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
