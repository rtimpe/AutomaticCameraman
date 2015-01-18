#include "BaseTracker.h"


////////////////////////////////////////////////////////////////////////////////

BaseTracker::BaseTracker(int id) {
	_numTrackers = 0;
	_id = id;
}


BaseTracker::~BaseTracker() {
	for (int i=0; i < _numTrackers; i++) {
		delete _tr[i]->_clfparams->_ftrParams;
		delete _tr[i]->_clfparams;
		delete _tr[i]->_trparams;
		delete _tr[i];
	}
}


void BaseTracker::draw_short_term(IplImage *img){
	// Draw a rectangle representing the predicted location
	for (int t = 0; t < _numTrackers; ++t) {
		draw_rect(img, _tr[t]->_x*_tr[t]->_scale, _tr[t]->_y*_tr[t]->_scale, 
			_tr[t]->_w*_tr[t]->_scale, _tr[t]->_h*_tr[t]->_scale, 1, 
			255, 0, 0, 255);
	}
}		


bool BaseTracker::init_trackers(Tracker *tr[], int centerX[], int centerY[], 
	int dim[], float lRate[], int numTrackers, int numFeats, int numSelFeats, 
	IplImage *gray) {
	TrackerParams *trparams;
	ClfParams *clfparams;
	FtrParams *ftrparams;
	Matrixu initImg;

	// Create tracker and parameters
	for (int i=0; i < numTrackers; i++) {
		tr[i] = new Tracker();
		trparams = new TrackerParams();
		clfparams = new ClfParams();
		ftrparams = new FtrParams();

		float scale = dim[i]/((float)TRACKING_WIN_DIM);

		tr[i]->_avg = 0;
		tr[i]->_avg2 = 0;
		tr[i]->_count = 0;

		ftrparams->_minNumRect = 2;
		ftrparams->_maxNumRect = 6;
		ftrparams->_width = TRACKING_WIN_DIM; 	// FPGA will always need to 
		ftrparams->_height = TRACKING_WIN_DIM;	// calculate using this size

		clfparams->_numSel = numSelFeats;
		clfparams->_numFeat = numFeats;
		clfparams->_lRate = lRate[i];
		clfparams->_ftrParams = ftrparams;

		trparams->_init_negnumtrain	= 65;
		trparams->_init_postrainrad	= 3.0f;
		trparams->_srchwinsz = 35;//min((float)MAX_SEARCH_RADIUS, MIN_SEARCH_RADIUS + 4*MIN_SEARCH_RADIUS*expf(-scale));
		trparams->_negnumtrain = 65;
		trparams->_posradtrain = 4;//max(2.0f, 4.0f/log2f(max(2.0f, log2f(scale))));
		trparams->_initScale = scale;
		trparams->_initX = max(0.0f, centerX[i]/scale - TRACKING_WIN_DIM/2);	
		trparams->_initY = max(0.0f, centerY[i]/scale - TRACKING_WIN_DIM/2);
		trparams->_initW = TRACKING_WIN_DIM;		// FPGA will always need to 
		trparams->_initH = TRACKING_WIN_DIM;		// calculate using this size
		trparams->_tpl = cvCreateImage(cvSize(TRACKING_WIN_DIM, TRACKING_WIN_DIM), IPL_DEPTH_8U, 1);
		trparams->_tplSqSum = 0;

		if (i == 1) {
			trparams->_boxcolor[0] = 25;
			trparams->_boxcolor[1] = 25;
			trparams->_boxcolor[2] = 204;
		}
		else if (i == 2) {
			trparams->_boxcolor[0] = 25;
			trparams->_boxcolor[1] = 204;
			trparams->_boxcolor[2] = 25;
		}

		// Initialize on the initial frame
		IplImage* tmp = cvCreateImage(
			cvSize(gray->width/(float)trparams->_initScale, gray->height/(float)trparams->_initScale), 
			IPL_DEPTH_8U, 1); 
		cvResize(gray, tmp);
		initImg.Resize(tmp->height, tmp->width);
		initImg.GrayIplImage2Matrix(tmp);
		tr[i]->init(&initImg, trparams, clfparams);
		cvReleaseImage(&tmp);
	}

	return true;
}


bool BaseTracker::init(int centerX, int centerY, int dim, int numFeats, 
	int numSelFeats, IplImage *gray) {
	// Initialize locals
	int dimArr[3] = { dim*pow(2, -1), dim*pow(1.5, -1), dim };
	int xArr[3] = { centerX - 3, centerX + 3, centerX };
	int yArr[3] = { centerY - 3, centerY + 3, centerY };
	float lRateArr[3] = { 0.45f, 0.65f, 0.85f };
	_numTrackers = 3;

	// Initialize the Tracker
	if (!init_trackers(_tr, xArr, yArr, dimArr, lRateArr, _numTrackers, numFeats, numSelFeats, gray))
		return false;

	return true;
}


inline bool ensure_trackers_agree(Tracker *tr[], float widthPercent, int *cX, int *cY) {
	int x, y;
	bool rtn = true;

	int distThresh = (widthPercent*tr[2]->_w*tr[2]->_scale)*(widthPercent*tr[2]->_w*tr[2]->_scale);
	int dist01 = DIST((int)((tr[0]->_x + tr[0]->_w/2)*tr[0]->_scale), (int)((tr[0]->_y + tr[0]->_h/2)*tr[0]->_scale), 
		(int)((tr[1]->_x + tr[1]->_w/2)*tr[1]->_scale), (int)((tr[1]->_y + tr[1]->_h/2)*tr[1]->_scale));
	int dist02 = DIST((int)((tr[0]->_x + tr[0]->_w/2)*tr[0]->_scale), (int)((tr[0]->_y + tr[0]->_h/2)*tr[0]->_scale), 
		(int)((tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale), (int)((tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale));
	int dist12 = DIST((int)((tr[1]->_x + tr[1]->_w/2)*tr[1]->_scale), (int)((tr[1]->_y + tr[1]->_h/2)*tr[1]->_scale),
		(int)((tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale), (int)((tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale));
	if (dist01 > distThresh || dist02 > distThresh || dist12 > distThresh) {
		if (dist01 < dist02) {
			if (dist01 < dist12) {
				x = ((tr[0]->_x +  tr[0]->_w/2)*tr[0]->_scale + (tr[1]->_x + tr[1]->_w/2)*tr[1]->_scale)/2;
				y = ((tr[0]->_y +  tr[0]->_h/2)*tr[0]->_scale + (tr[1]->_y + tr[1]->_h/2)*tr[1]->_scale)/2;
				tr[2]->_x = max(0.0f, x/tr[2]->_scale - tr[2]->_w/2);
				tr[2]->_y = max(0.0f, y/tr[2]->_scale - tr[2]->_h/2);
				tr[2]->_clfparams->_lRate = 0.0f;
				printf("\nCorrecting tracker %d to other trackers.\n", 2);
			}
			else {
				x = ((tr[1]->_x +  tr[1]->_w/2)*tr[1]->_scale + (tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale)/2;
				y = ((tr[1]->_y +  tr[1]->_h/2)*tr[1]->_scale + (tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale)/2;
				tr[0]->_x = max(0.0f, x/tr[0]->_scale - tr[0]->_w/2);
				tr[0]->_y = max(0.0f, y/tr[0]->_scale - tr[0]->_h/2);
				tr[0]->_clfparams->_lRate = 0.0f;
				printf("\nCorrecting tracker %d to other trackers.\n", 0);
			}
		}
		else {
			if (dist02 < dist12) {
				x = ((tr[0]->_x +  tr[0]->_w/2)*tr[0]->_scale + (tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale)/2;
				y = ((tr[0]->_y +  tr[0]->_h/2)*tr[0]->_scale + (tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale)/2;
				tr[1]->_x = max(0.0f, x/tr[1]->_scale - tr[1]->_w/2);
				tr[1]->_y = max(0.0f, y/tr[1]->_scale - tr[1]->_h/2);
				tr[1]->_clfparams->_lRate = 0.0f;
				printf("\nCorrecting tracker %d to other trackers.\n", 1);
			}
			else {
				x = ((tr[1]->_x +  tr[1]->_w/2)*tr[1]->_scale + (tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale)/2;
				y = ((tr[1]->_y +  tr[1]->_h/2)*tr[1]->_scale + (tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale)/2;
				tr[0]->_x = max(0.0f, x/tr[0]->_scale - tr[0]->_w/2);
				tr[0]->_y = max(0.0f, y/tr[0]->_scale - tr[0]->_h/2);
				tr[0]->_clfparams->_lRate = 0.0f;
				printf("\nCorrecting tracker %d to other trackers.\n", 0);
			}
		}
		rtn = false;
	}
	else {
		rtn = true;
	}

	// Return calculated center of all 3
	*cX = ((tr[0]->_x + tr[0]->_w/2)*tr[0]->_scale + (tr[1]->_x + tr[1]->_w/2)*tr[1]->_scale + (tr[2]->_x + tr[2]->_w/2)*tr[2]->_scale)/3;
	*cY = ((tr[0]->_y + tr[0]->_h/2)*tr[0]->_scale + (tr[1]->_y + tr[1]->_h/2)*tr[1]->_scale + (tr[2]->_y + tr[2]->_h/2)*tr[2]->_scale)/3;

	return rtn;
}


bool BaseTracker::trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
	IplImage *gray, IplImage *dispImg) {
	int centerX, centerY;
	float maxScore = 0;
	float lRateArr[3] = { 0.45f, 0.65f, 0.85f };
	float k = 0.0859f;
	float var;
	bool unstable = false;

	// Update all the trackers
	for (int t=0; t < 3; ++t) {
		var = _tr[t]->_avg2 - _tr[t]->_avg*_tr[t]->_avg;
		unstable = unstable || (var > 50) || (_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24) < _tr[t]->_avg);
		_tr[t]->_x = _tr[t]->_predX[NUM_PRED-1];
		_tr[t]->_y = _tr[t]->_predY[NUM_PRED-1];
		_tr[t]->_clfparams->_lRate = lRateArr[t];
		_tr[t]->_train = 1;
		_tr[t]->_avg = k*(_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24)) + (1.0-k)*_tr[t]->_avg;
		_tr[t]->_avg2 = k*(_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24))*(_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24)) + (1.0-k)*_tr[t]->_avg2;
		maxScore += _tr[t]->_predScore[NUM_PRED-1]/pow(2, 24);
	}
	maxScore = maxScore/3.0f;

	// Vote for agreement, update location if necessary
	ensure_trackers_agree(_tr, 0.3f, &centerX, &centerY);

	for (int t=0; t < 3; ++t) {
		cvRectangle(dispImg, 
			cvPoint(_tr[t]->_x*_tr[t]->_scale, _tr[t]->_y*_tr[t]->_scale), 
			cvPoint((_tr[t]->_x + _tr[t]->_w)*_tr[t]->_scale, (_tr[t]->_y + _tr[t]->_h)*_tr[t]->_scale), 
			cvScalar(255, 0, 0, 255), 1, 8, 0);
	}

	// Provide the prediction
	_cluster._confidence = maxScore;
	_cluster._num = 0;
	_cluster._stable = !unstable;
	_cluster._dim = _tr[0]->_w; // Use the middle one for the time being
	_cluster._x = centerX;
	_cluster._y = centerY;
	preds->push_back(&_cluster);

//	printf("    %d %d: trival: (%03d,%03d) %f(%f,%f) %f(%f,%f) %f(%f,%f) %d\n", _id, 0, centerX, centerY, 
//		_tr[0]->_predScore[NUM_PRED-1]/pow(2, 24), _tr[0]->_avg, _tr[0]->_avg2 - _tr[0]->_avg*_tr[0]->_avg,
//		_tr[1]->_predScore[NUM_PRED-1]/pow(2, 24), _tr[1]->_avg, _tr[1]->_avg2 - _tr[1]->_avg*_tr[1]->_avg,
//		_tr[2]->_predScore[NUM_PRED-1]/pow(2, 24), _tr[2]->_avg, _tr[2]->_avg2 - _tr[2]->_avg*_tr[2]->_avg, !unstable);

	return false;
}


bool BaseTracker::new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) {
	int cX, cY;

	// Draw the selected location
	cvRectangle(dispImg, cvPoint(cluster->_x, cluster->_y), cvPoint(cluster->_x, cluster->_y),
		cvScalar(0, 0, 255, 255), 2, 8, 0);

	// Move the trackers if we have a detection
	if (cluster->_num != 0) {
		// Get the center location of the trackers
		cX = ((_tr[0]->_x + _tr[0]->_w/2)*_tr[0]->_scale + (_tr[1]->_x + _tr[1]->_w/2)*_tr[1]->_scale + (_tr[2]->_x + _tr[2]->_w/2)*_tr[2]->_scale)/3;
		cY = ((_tr[0]->_y + _tr[0]->_h/2)*_tr[0]->_scale + (_tr[1]->_y + _tr[1]->_h/2)*_tr[1]->_scale + (_tr[2]->_y + _tr[2]->_h/2)*_tr[2]->_scale)/3;
		if (DIST((int)cluster->_x, (int)cluster->_y, cX, cY) > 7*7) {
			// Adjust by the shift amount
			printf("correcting from detection\n");
			for (int t=0; t < 3; ++t) {
				_tr[t]->_x += (cluster->_x - cX)/_tr[t]->_scale;
				_tr[t]->_y += (cluster->_y - cY)/_tr[t]->_scale;
			}
		}
	}

	return false;
}


