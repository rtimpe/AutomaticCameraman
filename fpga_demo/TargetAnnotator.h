#ifndef TARGET_ANNOTATOR_PUBLIC
#define TARGET_ANNOTATOR_PUBLIC

#include "Public.h"
#include "FrameAnnotator.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

class TargetAnnotator : public Annotator
{
public:
	TargetAnnotator(int id, TrackerController *controller);
	~TargetAnnotator(){};
	virtual bool draw_long_term(IplImage *img);
	virtual void draw_short_term(IplImage *img);

	TrackerController *_controller;						// TrackerController to indicate when tracking takes place
	int _histX[3];										// Previous x coordinates
	int _histY[3];										// Previous y coordinates
	int _pos;											// Position in coordinate history
	int _prevX;											// Previous x coordinate
	int _prevY;											// Previous y coordinate
};



#endif



