#if 0 // COMMENTED_OUT_FOR_MAC_BUILD
#include "BgSubTracker.h"


////////////////////////////////////////////////////////////////////////////////

BgSubTracker::BgSubTracker(int id, BgSub *bgSub) {
	_numTrackers = 0;
	_id = id;
	_prevX = 0;
	_prevY = 0;
	_bgSub = bgSub;
}


bool BgSubTracker::init(int x, int y, int rad, float thresh) {
	// Initialize locals
	_prevX = x;
	_prevY = y;
	_comX = x;
	_comY = y;
	_rad = rad;
	_thresh = thresh;
	_numConfid = 0;
	_avgConfid = 0;

	return true;
}


void BgSubTracker::draw_short_term(IplImage *img){
	// Draw a point representing the predicted location
	draw_rect(img, _prevX, _prevY, 2, 2, 2, 255, 128, 128, 255);
	draw_rect(img, _comX, _comY, 2, 2, 2, 255, 0, 0, 255);
}


void BgSubTracker::new_frame(Frame *frame) {
	// Update the backround subtraction mask
	_bgSub->update_mask(frame->_bgr, _prevX - _rad*3, _prevY - _rad*3, 2*_rad*3, 2*_rad*3, _thresh);
}


bool BgSubTracker::trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
	IplImage *gray, IplImage *dispImg) {
	static Cluster cluster;
	float fillPercent;

	// Get the center of mass of the background change in the tracking region
	_bgSub->get_center_of_mass(
		_prevX - _rad*1.5, 
		_prevY - _rad*1.5, 
		2*_rad*1.5, 
		2*_rad*1.5, 
		&cluster._x, &cluster._y, &fillPercent);

	// Update for drawing purposes
	_comX = cluster._x;
	_comY = cluster._y;

	// Provide the predition
	cluster._confidence = 0.0001;

	// Don't provide a prediction unless the fill percent is high enough
	if (fillPercent >= 0.05)
		preds->push_back(&cluster);
//printf("   bg:%f  fp:%f\n", fillPercent > 0.05 ? 0.0001 : 0, fillPercent);
	return false;
}


bool BgSubTracker::new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) {
	// Set the location to the one that was determined
	_prevX = cluster->_x;
	_prevY = cluster->_y;

	_avgConfid = _avgConfid + (cluster->_confidence - _avgConfid)/(++_numConfid);

	return false;
}


#endif // #if 0 // COMMENTED_OUT_FOR_MAC_BUILD
