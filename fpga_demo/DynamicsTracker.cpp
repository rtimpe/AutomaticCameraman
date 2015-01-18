#include "DynamicsTracker.h"


////////////////////////////////////////////////////////////////////////////////

DynamicsTracker::DynamicsTracker(int id) {
	_numTrackers = 0;
	_id = id;
}


bool DynamicsTracker::init(int x, int y) {
	_prevX = x;
	_prevY = y;
	_veloX = 0.0;
	_veloY = 0.0;
	_prevTime = 0.0;

	return true;
}


void DynamicsTracker::draw_short_term(IplImage *img){
	// Draw a point representing the predicted location
	draw_rect(img, _prevX, _prevY, 2, 2, 2, 0, 255, 0, 255);
}


bool DynamicsTracker::trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
	IplImage *gray, IplImage *dispImg) {
	static Cluster cluster;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	double time = (((double)tv.tv_sec*1000.0) + ((double)tv.tv_usec/1000.0));
	double delta = time - _prevTime;
	cluster._x = _prevX + 0.5f + _veloX*delta;
	cluster._y = _prevY + 0.5f + _veloY*delta;
	cluster._num = 1;
	cluster._confidence = 0.25;
	preds->push_back(&cluster);
//if (x != _prevX || y != _prevY)
//printf("suggesting: %d,%d, was %d,%d\n", cluster.x, cluster.y, _prevX, _prevY);

	return false;
}


bool DynamicsTracker::new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	double time = (((double)tv.tv_sec*1000.0) + ((double)tv.tv_usec/1000.0));
	double delta = time - _prevTime;
	_veloX = (cluster->_x - _prevX)/delta;
	_veloY = (cluster->_y - _prevY)/delta;
//if (x != _prevX || y != _prevY)
//printf("updated to: %d,%d, was %d,%d\n", x, y, _prevX, _prevY);
	_prevX = cluster->_x;
	_prevY = cluster->_y;
	_prevTime = time;

	return false;
}



