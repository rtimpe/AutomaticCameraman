#include "LibraryTracker.h"


////////////////////////////////////////////////////////////////////////////////
LibraryTracker::LibraryTracker(int id) : BaseTracker(id) {
	_numTrackers = 0;
}


inline void init_detector(Tracker *tr, int centerX, int centerY, int dim, IplImage *gray) {
	// Update the tracker's parameters
	tr->_scale = dim/(float)TRACKING_WIN_DIM;
	tr->_x = max(0, centerX - dim/2)/tr->_scale;
	tr->_y = max(0, centerY - dim/2)/tr->_scale;
	tr->_w = TRACKING_WIN_DIM;
	tr->_h = TRACKING_WIN_DIM;
	tr->_avg = 0;
	tr->_count = 0;
	tr->_trparams->_srchwinsz = 50;
	tr->_trparams->_tplSqSum = 0;
	tr->_clfparams->_lRate = 0;
	tr->_train = 1;
	tr->_trainCount = 0;

	// Crop a window patch at the selected location
	IplImage *tmp = cvCreateImage(cvSize(gray->width/tr->_scale, gray->height/tr->_scale), IPL_DEPTH_8U, 1); 
	cvResize(gray, tmp);
	cvSetImageROI(tmp, cvRect(tr->_x, tr->_y, TRACKING_WIN_DIM, TRACKING_WIN_DIM));
	cvCopy(tmp, tr->_trparams->_tpl, NULL);
	cvReleaseImage(&tmp);

	// Copy the window patch over to column-major.
	int ws = (((TRACKING_WIN_DIM+15)/16)*16);
	memset(tr->_trparams->_tplCol, 0, TRACKING_WIN_DIM*ws);
	for (int y=0; y<TRACKING_WIN_DIM; ++y) {
		for (int x=0; x<TRACKING_WIN_DIM; ++x) {
			tr->_trparams->_tplCol[(y*ws) + x] = (unsigned char)tr->_trparams->_tpl->imageData[(x*tr->_trparams->_tpl->widthStep) + y];
			tr->_trparams->_tplSqSum += 
				(unsigned char)tr->_trparams->_tpl->imageData[(y*tr->_trparams->_tpl->widthStep) + x]*
				(unsigned char)tr->_trparams->_tpl->imageData[(y*tr->_trparams->_tpl->widthStep) + x];
		}
	}

printf("adding detector: %d,%d %d %f\n", tr->_x, tr->_y, tr->_w, tr->_scale);
}


bool LibraryTracker::init(int centerX, int centerY, int dim, int numFeats, 
	int numSelFeats, IplImage *gray) {
	// Initialize locals
	int dimArr[10] = { dim };
	int xArr[10] = { centerX };
	int yArr[10] = { centerY };
	float lRateArr[10] = { 0.85f };
	for (int i=0; i < 10; ++i) {
		dimArr[i] = dim;
		xArr[i] = centerX;
		yArr[i] = centerY;
		lRateArr[i] = 0.85f;
	}

	// Initialize the Trackers
	if (!init_trackers(_tr, xArr, yArr, dimArr, lRateArr, 10, numFeats, numSelFeats, gray))
		return false;

	// Start a detector with this initial frame
	init_detector(_tr[0], centerX, centerY, dim, gray);
	_trainCount = 0;
	_detectionCount = 0;
	_numTrackers = 1;

	return true;
}


bool LibraryTracker::trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
	IplImage *gray, IplImage *dispImg) {
	float confid, zscore;
	int ndx;
	char resname[50];
	char tplname[50];
	int resNdxX;
	int resNdxY;
	double minVal; 
	double maxVal; 
	CvPoint minLoc; 
	CvPoint maxLoc;
	IplImage *res;
	IplImage *tmp;

	for (int t=0; t < _numTrackers; ++t) {
		// Update the location (may be corrected later)
		_tr[t]->_x = _tr[t]->_predX[NUM_PRED-1];
		_tr[t]->_y = _tr[t]->_predY[NUM_PRED-1];

		// Find the max prediction at least 5 pixels away from the max. Note,
		// the predictions are already sorted ascending.
		ndx = NUM_PRED-1;
		for (int j=NUM_PRED-2; j >= 0; --j) {
			if (DIST(_tr[t]->_predX[j], _tr[t]->_predY[j], 
					_tr[t]->_predX[NUM_PRED-1], _tr[t]->_predY[NUM_PRED-1]) >= 
						(5*_tr[t]->_scale)*(5*_tr[t]->_scale)) {
				ndx = j;
				break;
			}
		}
		confid = (_tr[t]->_predScore[NUM_PRED-1] - _tr[t]->_predScore[ndx])/
					sqrt(_tr[t]->_predVar);
		printf("    %d %d: nccval: (%03d,%03d) %f\n", _id, t, 
			_tr[t]->_nccX[NUM_PRED-1],
			_tr[t]->_nccY[NUM_PRED-1],
			_tr[t]->_nccScore[NUM_PRED-1]);

		// If max tracker and max NCC locations are close, use as a detection & train
		int nccDist = DIST(_tr[t]->_nccX[NUM_PRED-1], _tr[t]->_nccY[NUM_PRED-1], _tr[t]->_x, _tr[t]->_y);
		float nccDistThresh = (NCC_DIST/_tr[t]->_scale)*(NCC_DIST/_tr[t]->_scale);
		if (_tr[t]->_nccScore[NUM_PRED-1] > 0.98 || (_tr[t]->_nccScore[NUM_PRED-1] > 0.95 && nccDist < nccDistThresh)) {
			printf("    %d %d: tr val: (%03d,%03d) confid:%f score:%f avg:%f  1\n", _id, t, 
				(int)((_tr[t]->_x + _tr[t]->_w/2.0)*_tr[t]->_scale), 
				(int)((_tr[t]->_y + _tr[t]->_h/2.0)*_tr[t]->_scale), 
				confid, 
				_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24), 
				_tr[t]->_avg);

			_tr[t]->_x = _tr[t]->_nccX[NUM_PRED-1];
			_tr[t]->_y = _tr[t]->_nccY[NUM_PRED-1];
			_tr[t]->_clfparams->_lRate = 0.85f;
			_tr[t]->_train = 1;
			_tr[t]->_avg = _tr[t]->_avg + (confid - _tr[t]->_avg)/(++_tr[t]->_count);

			if (_tr[t]->_trainCount > TRAIN_THRESH) {
				cvRectangle(dispImg, 
					cvPoint(_tr[t]->_x*_tr[t]->_scale, _tr[t]->_y*_tr[t]->_scale), 
					cvPoint((_tr[t]->_x + _tr[t]->_w)*_tr[t]->_scale, (_tr[t]->_y + _tr[t]->_h)*_tr[t]->_scale), 
					cvScalar((50*_id+0) % 256, (20*_id+100) % 256, (176*_id+150) % 256, 255), 2, 8, 0);

				// Provide the prediction
				_clusters[t]._confidence = _tr[t]->_predScore[NUM_PRED-1]/pow(2, 24);
				_clusters[t]._num = 1;
				_clusters[t]._stable = false;
				_clusters[t]._dim = _tr[t]->_w*_tr[t]->_scale;
				_clusters[t]._x = (_tr[t]->_x + _tr[t]->_w/2.0)*_tr[t]->_scale;
				_clusters[t]._y = (_tr[t]->_y + _tr[t]->_h/2.0)*_tr[t]->_scale;
				preds->push_back(&_clusters[t]);
			}
			else {
				cvRectangle(dispImg, 
					cvPoint(_tr[t]->_x*_tr[t]->_scale, _tr[t]->_y*_tr[t]->_scale), 
					cvPoint((_tr[t]->_x + _tr[t]->_w)*_tr[t]->_scale, (_tr[t]->_y + _tr[t]->_h)*_tr[t]->_scale), 
					cvScalar((50*_id+0) % 256, (20*_id+100) % 256, (176*_id+150) % 256, 255), 1, 8, 0);
			}
		}
		else {
			printf("    %d %d: tr val: (%03d,%03d) confid:%f score:%f avg:%f  0\n", _id, t, 
				(int)((_tr[t]->_x + _tr[t]->_w/2.0)*_tr[t]->_scale), 
				(int)((_tr[t]->_y + _tr[t]->_h/2.0)*_tr[t]->_scale), 
				confid, 
				_tr[t]->_predScore[NUM_PRED-1]/pow(2, 24), 
				_tr[t]->_avg);

			_tr[t]->_train = 0;
		}

		sprintf(tplname, "tpl%02d%02d", _id, t);
		cvNamedWindow(tplname, CV_WINDOW_AUTOSIZE);
		cvShowImage(tplname, _tr[t]->_trparams->_tpl);
	}

	return false;
}


bool LibraryTracker::new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) {
	int d;
	bool changed = false;
	bool dtxn = false;

	// Update location to match selected (if not matched/training).
	for (int t=0; t < _numTrackers; ++t) {
		d = DIST((int)(cluster->_x/_tr[t]->_scale), (int)(cluster->_y/_tr[t]->_scale), 
				(_tr[t]->_x + _tr[t]->_w/2), (_tr[t]->_y + _tr[t]->_h/2));
		if (_tr[t]->_train == 0 || d > TRACKING_WIN_DIM*TRACKING_WIN_DIM) {
			_tr[t]->_train = 0;
			_tr[t]->_x = max(0.0f, cluster->_x/_tr[t]->_scale - _tr[t]->_w/2);
			_tr[t]->_y = max(0.0f, cluster->_y/_tr[t]->_scale - _tr[t]->_h/2);
		}
		else if (_tr[t]->_train == 1) {
			dtxn = true;
			_tr[t]->_trainCount++;
		}
	}

	// See if our current detector has been trained within the alloted time
	if (_trainCount > TRAIN_THRESH*2) {
		if (_numTrackers > 0 && _tr[_numTrackers-1]->_trainCount < TRAIN_THRESH) {
			_numTrackers--;
			changed = true;
printf("deleting detector %d\n", _numTrackers);
		}
	}
	else {
		++_trainCount;
	}

	if (!dtxn && _trainCount > TRAIN_THRESH*2)
		_detectionCount++;
	else
		_detectionCount = 0;

	// Add a new tracker when we're not training a tracker, we haven't used them
	// all yet, and when it's a good time to do so (this needs clarification).
	if (_trainCount > TRAIN_THRESH*2 && _detectionCount > 2 && _numTrackers < 10 && cluster->_stable) {
		init_detector(_tr[_numTrackers], cluster->_x, cluster->_y, cluster->_dim, gray);
		_trainCount = 0;
		_detectionCount = 0;
		_numTrackers++;
		changed = true;
	}

	return changed;
}


