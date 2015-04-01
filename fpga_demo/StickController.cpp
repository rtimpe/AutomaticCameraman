/*
 * StickController.cpp
 *
 *  Created on: Mar 18, 2015
 *      Author: robert
 */




#include "StickController.h"
#include "GridController.h"
#include <iostream>
#include <cmath>

using std::cout;
using std::endl;
using cv::Vec2d;
using std::vector;

StickController::StickController
(
    GridController *gc
)
: gc(gc), _end(false), p0(600, 400), p1(800, 400), tracking(false)
{

}

double dot(Vec2d v, Vec2d w) {
	return v[0] * w[0] + v[1] * w[1];
}

double squaredDist(Vec2d v, Vec2d w) {
	return (v[0] - w[0]) * (v[0] - w[0]) + (v[1] - w[1]) * (v[1] - w[1]);
}

// http://stackoverflow.com/questions/849211/shortest-distance-between-a-point-and-a-line-segment
double minimumDistance(Vec2d v, Vec2d w, Vec2d p) {
  // Return minimum distance between line segment vw and point p
  const double l2 = squaredDist(v, w);  // i.e. |w-v|^2 -  avoid a sqrt
  if (l2 == 0.0) return cv::norm(p - v);   // v == w case
  // Consider the line extending the segment, parameterized as v + t (w - v).
  // We find projection of point p onto the line.
  // It falls where t = [(p-v) . (w-v)] / |w-v|^2
  const double t = dot(p - v, w - v) / l2;
  if (t < 0.0) return cv::norm(p - v);       // Beyond the 'v' end of the segment
  else if (t > 1.0) return cv::norm(p - w);  // Beyond the 'w' end of the segment
  const Vec2d projection = v + t * (w - v);  // Projection falls on the segment
  return cv::norm(p - projection);
}


void* stickFunc(void *arg) {
	StickController *sc = (StickController *)arg;

	while (!sc->_end) {
		int numActivated = 0;

		Vec2d newP0(0,0);
		Vec2d newP1(0,0);
		int p0Count = 0;
		int p1Count = 0;
		double p0Tot = 0.0;
		double p1Tot = 0.0;
		for (int i = 0; i < sc->gc->_squares.size(); i++) {
			GridSquare &gs = sc->gc->_squares[i];

			Vec2d p(gs._x0, gs._y0);
			if (gs.occupied && minimumDistance(sc->p0, sc->p1, p) < 50) {
				numActivated++;
			}

			double coef = 1.0 / ((double) gs.timeOccupied / 10.0);
			if (gs.occupied && cv::norm(p - sc->p0) < 30) {
				newP0[0] += coef * p[0];
				newP0[1] += coef * p[1];
				p0Count++;
				p0Tot += coef;
			}
			if (gs.occupied && cv::norm(p - sc->p1) < 30) {
				newP1[0] += coef * p[0];
				newP1[1] += coef * p[1];
				p1Count++;
				p1Tot += coef;
			}
		}

		newP0 /= (double) p0Tot;
		newP1 /= (double) p1Tot;
		cout << p0Count << " " << p1Count << endl;
		if (numActivated >= 80) {
//			cout << newP0[0] << " " << newP0[1] << endl;
			sc->tracking = true;
			if (p0Count > 0) {
				sc->p0 = .8 * sc->p0 + .2 * newP0;
			}
			if (p1Count > 0) {
				sc->p1 = .8 * sc->p1 + .2 * newP1;
			}
		} else {
			sc->tracking = false;
			sc->p0[0] = 600;
			sc->p0[1] = 400;
			sc->p1[0] = 800;
			sc->p1[1] = 400;
		}

		usleep(1000);
	}

	return NULL;
}


void
StickController::start
(
    void
)
{
	_end = false;
	pthread_create(&_thread, NULL, stickFunc, this);

}


void
StickController::stop
(
    void
)
{
	cout << "called stop\n\n\n\n\n\n";
    _end = true;
}

StickController::~StickController(void)
{

}


