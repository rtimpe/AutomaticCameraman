#include "TargetAnnotator.h"
#include "MetaTracker.h"


////////////////////////////////////////////////////////////////////////////////

TargetAnnotator::TargetAnnotator(int id, TrackerController *controller) {
	_controller = controller;
	_id = id;
	_pos = 0;
	_prevX = -1;
	_prevY = -1;
}


bool TargetAnnotator::draw_long_term(IplImage *img) {
/*
	bool clear = true;

	// Initialize the previous locations
	if (_prevX < 0 || _prevY < 0) {
		_prevX = _controller->_x;
		_prevY = _controller->_y;
		for (int i=0; i < 3; ++i) {
			_histX[i] = _controller->_x;
			_histY[i] = _controller->_y;
		}
	}

	int avgX = _controller->_x; 
	int avgY = _controller->_y;
	for (int i=0; i < 3; ++i) {
		avgX += _histX[i];
		avgY += _histY[i];
	}

	// Draw a point representing the predicted location
	map<int, float>::iterator it = _controller->_attribs->find(MT_PEN_UP);
	if (it != _controller->_attribs->end() && it->second == 0.0) {
		draw_line(img, avgX/4, avgY/4, _prevX, _prevY, 2, 128, 128, 255, 255);
		clear = false;
	}

	// Update location
	_prevX = avgX/4;
	_prevY = avgY/4;
	_histX[_pos] = _controller->_x;
	_histY[_pos] = _controller->_y;
	_pos = (_pos == 2 ? 0 : _pos + 1);

	return clear;
*/
	return false;
}


void TargetAnnotator::draw_short_term(IplImage *img) {
	// Draw a point representing the predicted location
	draw_rect(img, _controller->_x, _controller->_y, 4, 4, 4, 0, 0, 255, 255);
}		


