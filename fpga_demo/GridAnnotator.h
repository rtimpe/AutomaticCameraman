#ifndef GRID_ANNOTATOR_PUBLIC
#define GRID_ANNOTATOR_PUBLIC

#include "Annotator.h"
#include "BallController.h"
#include "StickController.h"

class GridController;
class BallController;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
class GridAnnotator : public Annotator
{
public:

    GridAnnotator(int id, GridController * gc, BallController * bc, StickController * sc);
    virtual ~GridAnnotator(){};
    virtual bool draw_long_term(IplImage *img);
    virtual void draw_short_term(IplImage *img);

    GridController * _controller;
    BallController * ballController;
    StickController * stickController;
};


#endif // #define GRID_ANNOTATOR_PUBLIC
