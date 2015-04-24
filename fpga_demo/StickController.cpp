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
: gc(gc), _end(false), p0(600, 400), p1(800, 400), tracking(false), center(700, 400), theta(3.14159265 / 2.0)
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

double costFunc(std::vector<GridSquare> closeSquares, Vec2d p0, Vec2d p1) {
	int numActivated = 0;
	for (int i = 0; i < closeSquares.size(); i++) {
		GridSquare &gs = closeSquares[i];

		Vec2d p(gs._x0, gs._y0);
		if (gs.occupied && minimumDistance(p0, p1, p) < 20) {
			numActivated++;
		}

//		double coef = 1.0 / ((double) gs.timeOccupied / 10.0);
//		if (gs.occupied && cv::norm(p - sc->p0) < 30) {
//			newP0[0] += coef * p[0];
//			newP0[1] += coef * p[1];
//			p0Count++;
//			p0Tot += coef;
//		}
//		if (gs.occupied && cv::norm(p - sc->p1) < 30) {
//			newP1[0] += coef * p[0];
//			newP1[1] += coef * p[1];
//			p1Count++;
//			p1Tot += coef;
//		}
	}
	double cost = std::numeric_limits<double>::max();
	if (numActivated != 0) {
		cost = 1.0 / numActivated;
	}
	return cost;
}

std::vector<Vec2d> computeEndpoints(Vec2d center, double theta, int len) {
	Vec2d p0, p1;
	p0[0] = -1.0 * sin(theta);
	p0[1] = cos(theta);
	p0 *= len;
	p0 += center;
	p1[0] = sin(theta);
	p1[1] = -1.0 * cos(theta);
	p1 *= len;
	p1 += center;
	std::vector<Vec2d> ret;
	ret.push_back(p0);
	ret.push_back(p1);
	return ret;
}

void* stickFunc(void *arg) {
	StickController *sc = (StickController *)arg;

	while (!sc->_end) {
		std::vector<GridSquare> closeSquares;
		for (int i = 0; i < sc->gc->_squares.size(); i++) {
			GridSquare &gs = sc->gc->_squares[i];

			Vec2d p(gs._x0, gs._y0);
			if (cv::norm(sc->center - p) < 300) {
				closeSquares.push_back(gs);
			}
		}
		double minCost = std::numeric_limits<double>::max();

		for (int i = -30; i < 30; i += 10) {
			for (int j = -30; j < 30; j += 10) {
				for (double t = -3.14159 / 4.0; t < 3.14159 / 4.0; t += .6) {
					Vec2d offset(i, j);
					Vec2d newCenter = sc->center + offset;
					double newTheta = sc->theta + t;
					std::vector<Vec2d> pts = computeEndpoints(newCenter, newTheta, sc->len);
					if (pts[0][0] < 0 || pts[0][0] > sc->gc->_img_w || pts[0][1] < 0 || pts[0][1] > sc->gc->_img_h ||
							pts[1][0] < 0 || pts[1][0] > sc->gc->_img_w || pts[1][1] < 0 || pts[1][1] > sc->gc->_img_h) {
						continue;
					}

					double cost = costFunc(closeSquares, pts[0], pts[1]);
					if (cost < std::numeric_limits<double>::max()) {
						cout << cost << endl;
					}
					if (cost < minCost) {
						minCost = cost;
						sc->center = newCenter;
						sc->theta = newTheta;
					}
				}
			}
		}

		std::vector<Vec2d> pts = computeEndpoints(sc->center, sc->theta, sc->len);
		sc->p0 = pts[0];
		sc->p1 = pts[1];
//		sc->theta += .0001;
//		int numActivated = 0;
//
//		Vec2d newP0(0,0);
//		Vec2d newP1(0,0);
//		int p0Count = 0;
//		int p1Count = 0;
//		double p0Tot = 0.0;
//		double p1Tot = 0.0;
//		for (int i = 0; i < sc->gc->_squares.size(); i++) {
//			GridSquare &gs = sc->gc->_squares[i];
//
//			Vec2d p(gs._x0, gs._y0);
//			if (gs.occupied && minimumDistance(sc->p0, sc->p1, p) < 50) {
//				numActivated++;
//			}
//
//			double coef = 1.0 / ((double) gs.timeOccupied / 10.0);
//			if (gs.occupied && cv::norm(p - sc->p0) < 30) {
//				newP0[0] += coef * p[0];
//				newP0[1] += coef * p[1];
//				p0Count++;
//				p0Tot += coef;
//			}
//			if (gs.occupied && cv::norm(p - sc->p1) < 30) {
//				newP1[0] += coef * p[0];
//				newP1[1] += coef * p[1];
//				p1Count++;
//				p1Tot += coef;
//			}
//		}
//
//		newP0 /= (double) p0Tot;
//		newP1 /= (double) p1Tot;
//		cout << p0Count << " " << p1Count << endl;
//		double alpha = 0.7;
//		if (numActivated >= 40) {
//			sc->tracking = true;
//			if (p0Count > 0 && cv::norm(newP1 - newP0) > 80) {
//				sc->p0 = alpha * sc->p0 + (1.0 - alpha) * newP0;
//			}
//			if (p1Count > 0 && cv::norm(newP1 - newP0) > 80) {
//				sc->p1 = alpha * sc->p1 + (1.0 - alpha) * newP1;
//			}
//		} else {
//			sc->tracking = false;
//			sc->p0[0] = 600;
//			sc->p0[1] = 400;
//			sc->p1[0] = 800;
//			sc->p1[1] = 400;
//		}

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


