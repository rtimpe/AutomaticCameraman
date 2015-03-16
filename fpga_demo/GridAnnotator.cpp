#include "GridAnnotator.h"
#include "GridController.h"
#include "BallController.h"


GridAnnotator::GridAnnotator
(
    int id,
    GridController * gc,
	BallController * bc
)
:_controller(gc), ballController(bc)
{
    _id = id;
}


bool
GridAnnotator::draw_long_term
(
    IplImage *img
)
{
    // Draw grid of squares
    SquareIter end = _controller->_squares.end();
    for (SquareIter it = _controller->_squares.begin();
         end != it; ++it)
    {
        GridSquare & square = *it;
        if (square.occupied) {
        	draw_rect(img, square._x0, square._y0, square._w, square._h, 1, 255, 0, 0, 255);
        }
        //else {
        // 	draw_rect(img, square._x0, square._y0, square._w, square._h, 1, 0, 255, 0, 255);
        //}
    }

    if (ballController->hit) {
    	draw_circle(img, ballController->xPos, ballController->yPos, ballController->radius, 3, 0, 255, 255, 255);
    } else {
    	draw_circle(img, ballController->xPos, ballController->yPos, ballController->radius, 3, 255, 0, 255, 255);
    }

    return true;
}


void
GridAnnotator::draw_short_term
(
    IplImage *img
)
{
}
