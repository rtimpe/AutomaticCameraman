#ifndef GRID_CONTROLLER_PUBLIC
#define GRID_CONTROLLER_PUBLIC

#include "Public.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////

struct GridSquare
{
public:
    GridSquare(int x0, int y0, int w, int h);

    int _x0;   // location of upper right corner
    int _y0;   // location of upper left corner
    int _w;    // width
    int _h;    // height
};


class GridController
{
    friend class GridAnnotator;

public:
	GridController(int upper_x, int upper_y, int x_step, int y_step, int dim,
	               int img_w, int img_h);
	~GridController();
	void start();

protected:
	int _upper_x;  // x offset of upper left square
	int _upper_y;  // y offset of upper left square
	int _x_step;   // number of pixels between left edge of each square
	int _y_step;   // number of pixels between top edge of each square
	int _dim;      // height and width of a single box
    int _img_w;    // width of image in pixels
    int _img_h;    // height of image in pixels

	std::vector<GridSquare> _squares; // grid of squares
};


typedef std::vector<GridSquare>::const_iterator CSquareIter;
typedef std::vector<GridSquare>::iterator SquareIter;


#endif // #define GRID_CONTROLLER_PUBLIC



