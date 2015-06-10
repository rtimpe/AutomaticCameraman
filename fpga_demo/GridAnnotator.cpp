#include "GridAnnotator.h"
#include "GridController.h"
#include "BallController.h"


GridAnnotator::GridAnnotator
(
    int id,
    GridController * gc,
	BallController * bc,
	StickController * sc
)
:_controller(gc), ballController(bc), stickController(sc)
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
        if (square.timeOccupied < 0) {
        	continue;
        }

        cv::Vec2d p(square._x0, square._y0);
        double dist = cv::norm(p - stickController->center);
        if (square.occupied && dist < stickController->proximity) {
        	draw_rect(img, square._x0, square._y0, square._w, square._h, 1, 255, 0, 0, 255);
        }
//        else {
//        	draw_rect(img, square._x0, square._y0, square._w, square._h, 1, 0, 255, 0, 255);
//        }

    }

	//if (stickController->tracking) {
	//	draw_line(img, stickController->p0[0], stickController->p0[1], stickController->p1[0], stickController->p1[1], 5, 0, 255, 255, 255);
	//} else {
	//	draw_line(img, stickController->p0[0], stickController->p0[1], stickController->p1[0], stickController->p1[1], 5, 0, 0, 255, 255);
	//}

    draw_line(img, stickController->p0[0], stickController->p0[1], stickController->p1[0], stickController->p1[1], 5,
              stickController->_stick_color(0), stickController->_stick_color(1), stickController->_stick_color(2), stickController->_stick_color(3));



//    if (ballController->hit) {
//    	draw_circle(img, ballController->xPos, ballController->yPos, ballController->radius, 3, 0, 255, 255, 255);
//    } else {
//    	draw_circle(img, ballController->xPos, ballController->yPos, ballController->radius, 3, 255, 0, 255, 255);
//    }

    return true;
}


void
GridAnnotator::draw_short_term
(
    IplImage *img
)
{
}
