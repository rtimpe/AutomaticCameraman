#ifndef ANNOTATOR_PUBLIC
#define ANNOTATOR_PUBLIC

#include "Public.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////
class Annotator
{
public:
	virtual ~Annotator(){};
	virtual bool draw_long_term(IplImage *img) = 0;
	virtual void draw_short_term(IplImage *img) = 0;
	void draw_rect(IplImage *img, int x, int y, int w, int h, int lineWidth, 
		int R, int G, int B, int alpha);
	void draw_line(IplImage *img, int x0, int y0, int x1, int y1, int lineWidth, 
		int R, int G, int B, int alpha);

	int _id;											// Annotator id
};

#endif



