#ifndef DYNAMICS_TRACKER_PUBLIC
#define DYNAMICS_TRACKER_PUBLIC

#include "Public.h"
#include "MetaTracker.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class DynamicsTracker : public MetaTracker
{
public:
	DynamicsTracker(int id);
	~DynamicsTracker(){};
	bool init(int x, int y);
	virtual bool draw_long_term(IplImage *img) { return false; };
	virtual void draw_short_term(IplImage *img);
	virtual void new_frame(Frame *frame) {};
	virtual bool trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
		IplImage *gray, IplImage *dispImg);
	virtual bool new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg);

	double _prevTime;									// Previous time in ms
	int _prevX;											// Previous x position
	int _prevY;											// Previous y position
	double _veloX;										// X position velocity
	double _veloY;										// Y position velocity
};



#endif



