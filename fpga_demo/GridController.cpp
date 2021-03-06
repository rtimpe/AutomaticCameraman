#include "GridController.h"
#include "FramePool.h"
#include <iostream>
#include <cmath>

using std::cout;
using std::endl;


static void * controller_function(void * args);


GridSquare::GridSquare
(
    int x0,
    int y0,
    int w,
    int h
)
: _x0(x0), _y0(y0), _w(w), _h(h), runningMeanR(0.0), runningMeanSquaredR(0.0),
  runningMeanG(0.0), runningMeanSquaredG(0.0), runningMeanB(0.0), runningMeanSquaredB(0.0),
  meanShortR(0.0), meanShortG(0.0), meanShortB(0.0), occupied(false)
{}


void
GridSquare::reset
(
    void
)
{
    runningMeanR = 0.0;
    runningMeanSquaredR = 0.0;
    runningMeanG = 0.0;
    runningMeanSquaredG =0.0;
    runningMeanB =0.0;
    runningMeanSquaredB = 0.0;
    meanShortR = 0.0;
    meanShortG = 0.0;
    meanShortB = 0.0;
    occupied = false;
}



GridController::GridController
(
    int upper_x,
    int upper_y,
    int x_step,
    int y_step,
    int dim,
    int img_w,
    int img_h,
	FramePool *videoPool,
	double shortAlpha,
	double longAlpha,
	double diff
)
: _upper_x(upper_x), _upper_y(upper_y), _x_step(x_step), _y_step(y_step),
  _dim(dim), _img_w(img_w), _img_h(img_h), videoPool(videoPool), shortAlpha(shortAlpha),
  longAlpha(longAlpha), diff(diff), _end(false), timer(200)
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

void GridController::reset(void)
{
    for (int i = 0; i < _squares.size(); i++) {
        GridSquare &gs = _squares[i];

        gs.reset();
    }
}


void* trackerFunc(void *arg) {

	GridController *gridController = (GridController *)arg;
	Frame *frame = NULL;
	long frameNum = 0;
	bool isFirstFrame = true;
	long firstFrameNum = 0;

	while (!gridController->_end) {
		int newFrameNum = gridController->videoPool->acquire(&frame, frameNum, false, true);

		if (!frame || newFrameNum <= frameNum)
		{
		    usleep(5000);
		    continue;
		}

		frameNum = newFrameNum;

		if (isFirstFrame)
		{
		    firstFrameNum = frameNum;
		    isFirstFrame = false;
		}

		cv::Mat img(frame->_bgr);
		//cv::Mat img;
		//cv::cvtColor(imgColor, img, CV_RGB2GRAY);

		bool occupiedChange = false;
		// compute new average value for each square
		for (int i = 0; i < gridController->_squares.size(); i++) {
			GridSquare &gs = gridController->_squares[i];
			double averageR = 0.0;
			double averageG = 0.0;
			double averageB = 0.0;
			for (int j = gs._x0; j < gs._x0 + gs._w; j++) {
				for (int k = gs._y0; k < gs._y0 + gs._h; k++) {
					//cout << "start j: " << j << " k: " << k << endl;
					cv::Vec3b pixel = img.at<cv::Vec3b>(k, j);
					//cout << "end\n";
					averageB += pixel[0];
					averageG += pixel[1];
					averageR += pixel[2];
				}
			}
			averageR /= (double) (gs._w * gs._h);
			averageG /= (double) (gs._w * gs._h);
			averageB /= (double) (gs._w * gs._h);


			if (frameNum == firstFrameNum) {
				gs.runningMeanB = averageB;
				gs.runningMeanR = averageR;
				gs.runningMeanG = averageG;
				gs.meanShortB = averageB;
				gs.meanShortR = averageR;
				gs.meanShortG = averageG;
				gs.runningMeanSquaredB = averageB * averageB;
				gs.runningMeanSquaredR = averageR * averageR;
				gs.runningMeanSquaredG = averageG * averageG;
			} else {
				if (!gs.occupied) {
					double alpha = gridController->longAlpha;
					gs.runningMeanR = (1.0 - alpha) * gs.runningMeanR + alpha * averageR;
					gs.runningMeanSquaredR = (1.0 - alpha) * gs.runningMeanSquaredR + alpha * averageR * averageR;
					gs.runningMeanG = (1.0 - alpha) * gs.runningMeanG + alpha * averageG;
					gs.runningMeanSquaredG = (1.0 - alpha) * gs.runningMeanSquaredG + alpha * averageG * averageG;
					gs.runningMeanB = (1.0 - alpha) * gs.runningMeanB + alpha * averageB;
					gs.runningMeanSquaredB = (1.0 - alpha) * gs.runningMeanSquaredB + alpha * averageB * averageB;
				}

				double smallAlpha = gridController->shortAlpha;
				gs.meanShortB = (1.0 - smallAlpha) * gs.meanShortB + smallAlpha * averageB;
				gs.meanShortG = (1.0 - smallAlpha) * gs.meanShortG + smallAlpha * averageG;
				gs.meanShortR = (1.0 - smallAlpha) * gs.meanShortR + smallAlpha * averageR;

				double stdR = std::sqrt(gs.runningMeanSquaredR - gs.runningMeanR * gs.runningMeanR);
				double stdG = std::sqrt(gs.runningMeanSquaredG - gs.runningMeanG * gs.runningMeanG);
				double stdB = std::sqrt(gs.runningMeanSquaredB - gs.runningMeanB * gs.runningMeanB);

				double diffR = std::abs(gs.meanShortR - gs.runningMeanR);
				double diffG = std::abs(gs.meanShortG - gs.runningMeanG);
				double diffB = std::abs(gs.meanShortB - gs.runningMeanB);
				//cout << "long: " << gs.runningMeanR << " short: " << gs.meanShortR << " average: " << averageR << " std: " << stdR << endl;
				double diff = gridController->diff;
				if (frameNum > 500 + firstFrameNum) {
					if (diffR > diff * stdR || diffG > diff * stdG || diffB > diff * stdB) {
						if (gs.occupied == false) {
							occupiedChange = true;
						}
						gs.occupied = true;
					} else {
						gs.occupied = false;
					}
				}
//				if (gs._x0 < 1000 && gs._x0 > 900) {
//					gs.occupied = true;
//				}
//				if (gs._x0 < 200 && gs._x0 > 100) {
//					gs.occupied = true;
//				}
			}
		}

		if (occupiedChange) {
			gridController->timer = 200;
		} else if (frameNum > 500 + firstFrameNum) {
			gridController->timer--;
			if (gridController->timer < 0) {
				gridController->timer = 200;
				cout << "reseting\n";
				for (int i = 0; i < gridController->_squares.size(); i++) {
					GridSquare &gs = gridController->_squares[i];
					double averageR = 0.0;
					double averageG = 0.0;
					double averageB = 0.0;
					for (int j = gs._x0; j < gs._x0 + gs._w; j++) {
						for (int k = gs._y0; k < gs._y0 + gs._h; k++) {
							//cout << "start j: " << j << " k: " << k << endl;
							cv::Vec3b pixel = img.at<cv::Vec3b>(k, j);
							//cout << "end\n";
							averageB += pixel[0];
							averageG += pixel[1];
							averageR += pixel[2];
						}
					}
					averageR /= (double) (gs._w * gs._h);
					averageG /= (double) (gs._w * gs._h);
					averageB /= (double) (gs._w * gs._h);


					gs.runningMeanB = averageB;
					gs.runningMeanR = averageR;
					gs.runningMeanG = averageG;
					gs.meanShortB = averageB;
					gs.meanShortR = averageR;
					gs.meanShortG = averageG;
					gs.runningMeanSquaredB = averageB * averageB;
					gs.runningMeanSquaredR = averageR * averageR;
					gs.runningMeanSquaredG = averageG * averageG;

					gs.occupied = false;
					isFirstFrame = true;
				}
			}
		}

		gridController->videoPool->release(frame);
	}

	return NULL;
}

void
GridController::start
(
    void
)
{
    _end = false;
	pthread_create(&_thread, NULL, trackerFunc, this);
}


void
GridController::stop
(
    void
)
{
	cout << "called stop\n\n\n\n\n\n";
    _end = true;
	pthread_join(_thread, NULL);
}

GridController::~GridController
(
    void
)
{

}


