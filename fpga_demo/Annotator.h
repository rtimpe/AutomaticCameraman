#ifndef ANNOTATOR_PUBLIC
#define ANNOTATOR_PUBLIC

#include "Public.h"
#include <string.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////
class Annotator
{
public:
	virtual ~Annotator(){};
	virtual bool draw_long_term(IplImage *img) = 0;
	virtual void draw_short_term(IplImage *img) = 0;
	void draw_rect(IplImage *img, int x, int y, int w, int h, int lineWidth, 
		int R, int G, int B, int alpha);
	void draw_circle(IplImage *img, int x, int y, int radius, int width, int R, int G, int B, int alpha);
	void draw_line(IplImage *img, int x0, int y0, int x1, int y1, int lineWidth, 
		int R, int G, int B, int alpha);
    void draw_text(IplImage *img, int x, int y, const std::string & s, int fontFace, double scale,
                   const cv::Scalar & color, int thickness, int lineType);

	int _id;											// Annotator id
};

#endif



