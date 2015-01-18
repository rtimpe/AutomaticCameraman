#include "BoostedTracker.h"


////////////////////////////////////////////////////////////////////////////////

BoostedTracker::BoostedTracker(int id, BgSub *bgSub) {
	_numTrackers = 0;
	_id = id;
	_bgSub = bgSub;
}


BoostedTracker::~BoostedTracker() {
	for (int i=0; i < _numTrackers; i++) {
		delete _tr[i]->_clfparams->_ftrParams;
		delete _tr[i]->_clfparams;
		delete _tr[i]->_trparams;
		delete _tr[i];
	}
}


void BoostedTracker::draw_short_term(IplImage *img){
	// Draw a point representing the predicted location
	for (int i = 0; i < _numTrackers; i++) {
		if (_detected[i]) {
			draw_rect(img, _tr[i]->_x*_tr[i]->_scale, _tr[i]->_y*_tr[i]->_scale, 
				_tr[i]->_w*_tr[i]->_scale, _tr[i]->_h*_tr[i]->_scale, 1, 
				0, 255, 0, 255);
		}
	}
}		


bool BoostedTracker::init(int x, int y, int numFeat, int numSelFeat, int dim, 
	int numScales, float scaleStep, char *serTracker) {
	ifstream inFile, treeFile;
	int numStumps;
	float tmp;
	char path[100];

	// Load the Tracker
	for (int t=0; t < numScales; ++t) {
		inFile.open(serTracker);
		if (!inFile.is_open() || inFile.eof()) {
			printf("Cannot parse tracker file %s\n", serTracker);
			return false;
		}
		_tr[t] = new Tracker();
		_tr[t]->load(inFile);

		// Initialize the Tracker
		_tr[t]->_scale = _tr[t]->_scale * dim*pow(scaleStep, t)/_tr[t]->_w;
		_tr[t]->_x = x/_tr[t]->_scale - _tr[t]->_w/2;
		_tr[t]->_y = y/_tr[t]->_scale - _tr[t]->_h/2;
		_tr[t]->_trparams->_srchwinsz = 25*3.0;
		_tr[t]->_clfparams->_numSel = numSelFeat;
		_tr[t]->_clfparams->_numFeat = numFeat;
		_tr[t]->_train = 0;

		// Set the _sig1 value to -1 to indicate a boosted style classifer score
		for (int i=0; i < _tr[t]->_clf->_weakclf.size(); i++) {
			_tr[t]->_clf->_weakclf[i]->_sig1 = -1;
			_tr[t]->_clf->_weakclf[i]->_mu1 = 0;
			_tr[t]->_clf->_weakclf[i]->_e1 = 0;
		}

		// Load the boosted parameters
		sprintf(path, "%s.tree", serTracker);
		treeFile.open(path);
		if (!treeFile.is_open() || treeFile.eof()) {
			printf("Cannot parse tree file %s\n", path);
			return false;
		}

		treeFile>>numStumps;
		_tr[t]->_clf->_selectors.resize(numStumps);
		for (int c=0; c < numStumps; c++) {
			treeFile>>_tr[t]->_clf->_selectors[c];
			treeFile>>_tr[t]->_clf->_weakclf[_tr[t]->_clf->_selectors[c]]->_mu1;
			treeFile>>_tr[t]->_clf->_weakclf[_tr[t]->_clf->_selectors[c]]->_e1;
			treeFile>>tmp;
			_tr[t]->_clf->_weakclf[_tr[t]->_clf->_selectors[c]]->_e1 -= tmp;
		}	

		inFile.close();
		treeFile.close();
	}

	// Initialize locals
	_numTrackers = numScales;

	return true;
}


bool BoostedTracker::trackers_updated(vector<Cluster *> *preds, map<int, float> *attribs, 
	IplImage *gray, IplImage *dispImg) {
	float score;
	bool detected = false;

	for (int t=0; t < _numTrackers; ++t) {
		score = _tr[t]->_predScore[NUM_PRED-1]/16777216.0f;// 2^24
		printf("    maxscore: %.03f\n", score);
		_detected[t] = (score > 60);
		detected = detected || _detected[t]; 
	}

	// Update the attributes
	(*attribs)[MT_PEN_UP] = (detected ? 1.0 : 0.0);

	return false;
}


bool BoostedTracker::new_location(Cluster *cluster, IplImage *gray, IplImage *dispImg) {
	float sX, sY;

	for (int t=0; t < _numTrackers; ++t) {
		sX = cluster->_x/_tr[t]->_scale; // Bring x, y coordinates into 
		sY = cluster->_y/_tr[t]->_scale; // resized coodinate space
		_tr[t]->_x = sX - _tr[t]->_w/2;
		_tr[t]->_y = sY - _tr[t]->_h/2;
	}

	return false;
}



