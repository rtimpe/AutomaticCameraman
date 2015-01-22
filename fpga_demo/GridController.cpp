#include "GridController.h"
#include <iostream>

using std::cout;
using std::endl;



GridSquare::GridSquare
(
    int x0,
    int y0,
    int w,
    int h
)
: _x0(x0), _y0(y0), _w(w), _h(h)
{}


GridController::GridController
(
    int upper_x,
    int upper_y,
    int x_step,
    int y_step,
    int dim,
    int img_w,
    int img_h
)
: _upper_x(upper_x), _upper_y(upper_y), _x_step(x_step), _y_step(y_step),
  _dim(dim), _img_w(img_w), _img_h(img_h)
{
    // Create grid of squares
    int x = _upper_x;
    while (x + _dim < _img_w)
    {
        int y = _upper_y;
        while (y + _dim < _img_h)
        {
            _squares.push_back(GridSquare(x, y, _dim, _dim));

            y += _y_step;
        }
        x += _x_step;
    }

    cout << "Image dimensions are " << _img_w << "x" << _img_h << endl;
    cout << "Number of squares is " << _squares.size() << endl;
}


void
GridController::start
(
    void
)
{

}


GridController::~GridController
(
    void
)
{

}

