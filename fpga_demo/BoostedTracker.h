#ifndef BOOSTED_TRACKER_PUBLIC
#define BOOSTED_TRACKER_PUBLIC

#include "Public.h"
#include "MetaTracker.h"
#include "BgSub.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class BoostedTracker : public MetaTracker
{
public:
	BoostedTracker(int id, BgSub *bgSub);
	~BoostedTracker();
	bool init(int x, int y, int numFeat, int numSelFeat, int dim, int numScales, 
		float scaleStep, char *serTracker);
	virtual bool draw_long_term(IplImage *img) { return false; };
	virtual void draw_short_term(IplImage *img);
	virtual void new_frame(Frame *frame) {};
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg);
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg);

	BgSub *_bgSub;										// Background subtraction
	bool _detected[MAX_NUM_TRACKERS];					// True if a detection is made
};



#endif



