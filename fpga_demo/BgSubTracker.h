#if 0 // COMMENTED_OUT_FOR_MAC_BUILD
#ifndef BACKGROUND_SUBTRACTION_TRACKER_PUBLIC
#define BACKGROUND_SUBTRACTION_TRACKER_PUBLIC

#include "Public.h"
#include "MetaTracker.h"
#include "BgSub.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class BgSubTracker : public MetaTracker
{
public:
	BgSubTracker(int id, BgSub *bgSub);
	~BgSubTracker(){};
	bool init(int x, int y, int rad, float thresh);
	virtual bool draw_long_term(IplImage *img) { return false; };
	virtual void draw_short_term(IplImage *img);
	virtual void new_frame(Frame *frame);
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg);
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg);

	BgSub *_bgSub;										// Background subtraction
	int _prevX;											// Previous location x coordinate
	int _prevY;											// Previous location y coordinate
	int _comX;											// Previous center of mass location x coordinate
	int _comY;											// Previous center of mass location y coordinate
	int _rad;											// Update radius
	float _thresh;										// Subtraction threshold
	float _avgConfid;	
	long _numConfid;
};



#endif



#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
