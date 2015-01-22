#ifndef GRID_ANNOTATOR_PUBLIC
#define GRID_ANNOTATOR_PUBLIC

#include "Annotator.h"

class GridController;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
class GridAnnotator : public Annotator
{
public:

    GridAnnotator(int id, GridController * gc);
    virtual ~GridAnnotator(){};
    virtual bool draw_long_term(IplImage *img);
    virtual void draw_short_term(IplImage *img);

    GridController * _controller;
};


#endif // #define GRID_ANNOTATOR_PUBLIC
