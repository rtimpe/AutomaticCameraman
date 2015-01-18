// MILTRACK
// Copyright 2009 Boris Babenko (bbabenko@cs.ucsd.edu | http://vision.ucsd.edu/~bbabenko).  Distributed under the terms of the GNU Lesser General Public License 
// (see the included gpl.txt and lgpl.txt files).  Use at own risk.  Please send me your feedback/suggestions/bugs.

#include "OnlineBoost.h"
#include "Tracker.h"
#include "Public.h"
#include "Sample.h"


bool Tracker::init(Matrixu *img, TrackerParams *p, ClfParams *clfparams)
{
	SampleSet posx, negx;
	int iiFree;

	iiFree = (!img->isInitII());

	if (!img->isInitII())
		img->initII();

	_clf = new ClfStrong();
	_clf->init(clfparams);

	_x = p->_initX;
	_y = p->_initY;
	_lastX = p->_initX;
	_lastY = p->_initY;
	_w = p->_initW;
	_h = p->_initH;
	_scale = p->_initScale;

	fprintf(stderr,"Initializing Tracker..\n");

	// sample positives and negatives from first frame
	posx.sampleImage(img, _x, _y, _w, _h, p->_init_postrainrad);
	negx.sampleImage(img, _x, _y, _w, _h, 2.0f*p->_srchwinsz, (1.5f*p->_init_postrainrad), p->_init_negnumtrain);
	if( posx.size()<1 || negx.size()<1 ) return false;

	// train
	_clf->update(posx, negx);
	posx.clear();
	negx.clear();

	if (iiFree)
		img->FreeII();

	_trparams = p;
	_clfparams = clfparams;
	_train = 1;

	return true;
}


float Tracker::update_location(Matrixu *img, int *locX, int *locY) {
	static SampleSet detectx;

	if (!img->isInitII())
		abortError(__LINE__,__FILE__,"Integral image not initialized before calling update_location");

	// run current clf on search window
	detectx.sampleImage(img, _x, _y, _w, _h, (float)_trparams->_srchwinsz);
	float resp = update_location(detectx, locX, locY);

	// clean up
	detectx.clear();
	return resp;
}


float Tracker::update_location(SampleSet &detectx, int *locX, int *locY) {
	static vectorf prob;
	float resp;

	// run current clf on search window
	prob = _clf->classify(detectx,_trparams->_useLogR);

	// find best location
	int bestind = max_idx(prob);
	resp=prob[bestind];
	if (locX != NULL && locY != NULL) {
		*locX = detectx[bestind]._col;
		*locY = detectx[bestind]._row; 
	}

	// clean up
	return resp;
}


void Tracker::update_classifier(Matrixu *img) {
	static SampleSet posx, negx;

	if (!img->isInitII())
		abortError(__LINE__,__FILE__,"Integral image not initialized before calling update_classifier");

	// train location clf (negx are randomly selected from image, posx is just the current tracker location)
	negx.sampleImage(img, _x, _y, _w, _h, (1.5f*_trparams->_srchwinsz), _trparams->_posradtrain+5, _trparams->_negnumtrain);
	posx.sampleImage(img, _x, _y, _w, _h, _trparams->_posradtrain, 0, _trparams->_posmaxtrain);
	update_classifier(posx, negx);

	// clean up
	posx.clear(); 
	negx.clear();
}


void Tracker::update_classifier(SampleSet &posx, SampleSet &negx) {
	_clf->update(posx, negx);
}


void Tracker::save(ofstream& os) {
	if (os.is_open()) {
		os<<_x<<endl;
		os<<_y<<endl;
		os<<_w<<endl;
		os<<_h<<endl;
		os<<_scale<<endl;
		os<<_train<<endl;
		os<<_trparams->_boxcolor[0]<<endl;
		os<<_trparams->_boxcolor[1]<<endl;
		os<<_trparams->_boxcolor[2]<<endl;
		os<<_trparams->_lineWidth<<endl;
		os<<_trparams->_negnumtrain<<endl;
		os<<_trparams->_init_negnumtrain<<endl;
		os<<_trparams->_posradtrain<<endl;
		os<<_trparams->_init_postrainrad<<endl;
		os<<_trparams->_posmaxtrain<<endl;
		os<<_trparams->_initX<<endl;
		os<<_trparams->_initY<<endl;
		os<<_trparams->_initW<<endl;
		os<<_trparams->_initH<<endl;
		os<<_trparams->_initScale<<endl;
		os<<_trparams->_srchwinsz<<endl;
		os<<_trparams->_weights[0]<<endl;
		os<<_trparams->_weights[1]<<endl;
		os<<_trparams->_weights[2]<<endl;
		os<<_trparams->_weights[3]<<endl;
		os<<_trparams->_weights[4]<<endl;
		os<<_trparams->_weights[5]<<endl;
		os<<_trparams->_weights[6]<<endl;
		os<<_trparams->_weights[7]<<endl;
		os<<_trparams->_weights[8]<<endl;
		os<<_clfparams->_lRate<<endl;
		os<<_clfparams->_numFeat<<endl;
		os<<_clfparams->_numSel<<endl;
		os<<_clfparams->_ftrParams->_width<<endl;
		os<<_clfparams->_ftrParams->_height<<endl;
		os<<_clfparams->_ftrParams->_maxNumRect<<endl;
		os<<_clfparams->_ftrParams->_minNumRect<<endl;
		os<<_clf->_numsamples<<endl;
		os<<_clf->_ftrs.size()<<endl;
		for (int i=0; i<_clf->_ftrs.size(); i++) {
			os<<_clf->_ftrs[i]->_width<<endl;
			os<<_clf->_ftrs[i]->_height<<endl;
			os<<_clf->_ftrs[i]->_weights.size()<<endl;
			for (int j=0; j<_clf->_ftrs[i]->_weights.size(); j++)
				os<<_clf->_ftrs[i]->_weights[j]<<endl;
			os<<_clf->_ftrs[i]->_rects.size()<<endl;
			for (int j=0; j<_clf->_ftrs[i]->_rects.size(); j++) {
				os<<_clf->_ftrs[i]->_rects[j].x<<endl;
				os<<_clf->_ftrs[i]->_rects[j].y<<endl;
				os<<_clf->_ftrs[i]->_rects[j].width<<endl;
				os<<_clf->_ftrs[i]->_rects[j].height<<endl;
			}
		}
		os<<_clf->_selectors.size()<<endl;
		for (int i=0; i<_clf->_selectors.size(); i++)
			os<<_clf->_selectors[i]<<endl;
		os<<_clf->_weakclf.size()<<endl;
		for (int i=0; i<_clf->_weakclf.size(); i++) {
			os<<_clf->_weakclf[i]->_mu0<<endl;
			os<<_clf->_weakclf[i]->_mu1<<endl;
			os<<_clf->_weakclf[i]->_sig0<<endl;
			os<<_clf->_weakclf[i]->_sig1<<endl;
			os<<_clf->_weakclf[i]->_e0<<endl;
			os<<_clf->_weakclf[i]->_e1<<endl;
			os<<_clf->_weakclf[i]->_n0<<endl;
			os<<_clf->_weakclf[i]->_n1<<endl;
			os<<_clf->_weakclf[i]->_trained<<endl;
			os<<_clf->_weakclf[i]->_ind<<endl;
			os<<_clf->_weakclf[i]->_lRate<<endl;
		}
	}
}


void Tracker::load(ifstream& is) {
	int sz0, sz1;
	if (is.is_open() && !is.eof()) {
		_trparams = new TrackerParams();
		_clfparams = new ClfParams();
		_clfparams->_ftrParams = new FtrParams();

		is>>_x;
		is>>_y;
		is>>_w;
		is>>_h;
		is>>_scale;
		is>>_train;
		_trparams->_boxcolor.resize(3);
		is>>_trparams->_boxcolor[0];
		is>>_trparams->_boxcolor[1];
		is>>_trparams->_boxcolor[2];
		is>>_trparams->_lineWidth;
		is>>_trparams->_negnumtrain;
		is>>_trparams->_init_negnumtrain;
		is>>_trparams->_posradtrain;
		is>>_trparams->_init_postrainrad;
		is>>_trparams->_posmaxtrain;
		is>>_trparams->_initX;
		is>>_trparams->_initY;
		is>>_trparams->_initW;
		is>>_trparams->_initH;
		is>>_trparams->_initScale;
		is>>_trparams->_srchwinsz;
		is>>_trparams->_weights[0];
		is>>_trparams->_weights[1];
		is>>_trparams->_weights[2];
		is>>_trparams->_weights[3];
		is>>_trparams->_weights[4];
		is>>_trparams->_weights[5];
		is>>_trparams->_weights[6];
		is>>_trparams->_weights[7];
		is>>_trparams->_weights[8];
		is>>_clfparams->_lRate;
		is>>_clfparams->_numFeat;
		is>>_clfparams->_numSel;
		is>>_clfparams->_ftrParams->_width;
		is>>_clfparams->_ftrParams->_height;
		is>>_clfparams->_ftrParams->_maxNumRect;
		is>>_clfparams->_ftrParams->_minNumRect;

		_clf = new ClfStrong();
		_clf->_params = _clfparams;

		is>>_clf->_numsamples;
		is>>sz0;
		_clf->_ftrs.resize(sz0);
		for (int i=0; i<_clf->_ftrs.size(); i++) {
			_clf->_ftrs[i] = new Ftr();
			is>>_clf->_ftrs[i]->_width;
			is>>_clf->_ftrs[i]->_height;
			is>>sz1;
			_clf->_ftrs[i]->_weights.resize(sz1);
			for (int j=0; j<_clf->_ftrs[i]->_weights.size(); j++)
				is>>_clf->_ftrs[i]->_weights[j];
			is>>sz1;
			_clf->_ftrs[i]->_rects.resize(sz1);
			for (int j=0; j<_clf->_ftrs[i]->_rects.size(); j++) {
				is>>_clf->_ftrs[i]->_rects[j].x;
				is>>_clf->_ftrs[i]->_rects[j].y;
				is>>_clf->_ftrs[i]->_rects[j].width;
				is>>_clf->_ftrs[i]->_rects[j].height;
			}
		}
		is>>sz0;
		_clf->_selectors.resize(sz0);
		for (int i=0; i<_clf->_selectors.size(); i++)
			is>>_clf->_selectors[i];
		is>>sz0;
		_clf->_weakclf.resize(sz0);
		for (int i=0; i<_clf->_weakclf.size(); i++) {
			_clf->_weakclf[i] = new ClfWeak();
			is>>_clf->_weakclf[i]->_mu0;
			is>>_clf->_weakclf[i]->_mu1;
			is>>_clf->_weakclf[i]->_sig0;
			is>>_clf->_weakclf[i]->_sig1;
			is>>_clf->_weakclf[i]->_e0;
			is>>_clf->_weakclf[i]->_e1;
			is>>_clf->_weakclf[i]->_n0;
			is>>_clf->_weakclf[i]->_n1;
			is>>_clf->_weakclf[i]->_trained;
			is>>_clf->_weakclf[i]->_ind;
			is>>_clf->_weakclf[i]->_lRate;
			_clf->_weakclf[i]->_ftrs = &_clf->_ftrs;
			_clf->_weakclf[i]->_ftr = _clf->_ftrs[_clf->_weakclf[i]->_ind];
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
TrackerParams::TrackerParams()
{
	_srchwinsz		= 30;
	_negsamplestrat	= 1;
	_boxcolor.resize(3);
	_boxcolor[0]	= 204;
	_boxcolor[1]	= 25;
	_boxcolor[2]	= 204;
	_lineWidth		= 2;
	_negnumtrain	= 15;
	_posradtrain	= 1;
	_posmaxtrain	= 100000;
	_init_negnumtrain = 1000;
	_init_postrainrad = 3;
	_initX			= 0;
	_initY			= 0;
	_initW			= 0;
	_initH			= 0;
	_initScale		= 1.0;
	_debugv			= false;
	_useLogR		= true;
	_disp			= true;
	_initWithFace	= true;
	_weights[0]		= 1.0f;
	_weights[1]		= 1.0f;
	_weights[2]		= 1.0f;
	_weights[3]		= 1.0f;
	_weights[4]		= 1.0f;
	_weights[5]		= 1.0f;
	_weights[6]		= 1.0f;
	_weights[7]		= 1.0f;
	_weights[8]		= 1.0f;
}


