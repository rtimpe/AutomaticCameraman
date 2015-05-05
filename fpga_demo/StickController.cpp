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
#include "StickEKF.h"

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

double costFunc(std::vector<GridSquare> closeSquares, Vec2d p0, Vec2d p1, Vec2d center) {
	double distTerm = 0.0;
	int numActivated = 0;
	for (int i = 0; i < closeSquares.size(); i++) {
		GridSquare &gs = closeSquares[i];

		double x = gs._x0 + (double)gs._w / 2.0;
		double y = gs._y0 + (double)gs._h / 2.0;
		Vec2d p(x, y);
		if (gs.occupied && minimumDistance(p0, p1, p) < 10) {
			numActivated++;
			double dist = cv::norm(p - center);
			if (dist == 0.0) {
				dist = 0.000000001;
			}
			distTerm += 1.0 / dist;
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
	if (numActivated > 5) {
		cost = 1.0 / numActivated;//distTerm;
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

	StickEKF sekf;
	static const double _P0[] = {1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
			0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
			0.0, 0.0, 0.0, 0.0, 0.0, 1.0};
	Vector x(6);
	x(1) = 700;
	x(2) = 0.0;
	x(3) = 400;
	x(4) = 0.0;
	x(5) = 3.14159 / 2.0;
	x(6) = 0.0;

	Matrix P0(6, 6, _P0);
	sekf.init(x, P0);

	cout << sekf.getX()(1) << endl;

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

		for (int i = -10; i <= 10; i += 5) {
			for (int j = -10; j <= 10; j += 5) {
//		for (int i=0; i < 3; i++) {
//			int newX = (i / 2) * (-1 * ((i % 2)) + 1) * 5;
//			for (int j=0; j < 3; j++) {
//				int newY = (j / 2) * (-1 * ((j % 2)) + 1) * 5;
				for (double t = -3.14159 / 10.0; t < 3.14159 / 10.0; t += .2) {
					Vec2d offset(i, j);
					Vec2d newCenter = sc->center + offset;
					double newTheta = sc->theta + t;
					std::vector<Vec2d> pts = computeEndpoints(newCenter, newTheta, sc->len);
					if (pts[0][0] < 0 || pts[0][0] > sc->gc->_img_w || pts[0][1] < 0 || pts[0][1] > sc->gc->_img_h ||
							pts[1][0] < 0 || pts[1][0] > sc->gc->_img_w || pts[1][1] < 0 || pts[1][1] > sc->gc->_img_h) {
						continue;
					}

					double cost = costFunc(closeSquares, pts[0], pts[1], newCenter);
					if (cost < minCost) {
						minCost = cost;
						sc->center = newCenter;
						sc->theta = newTheta;
					}
				}
			}
		}

		// kalman stuff
		Vector u;
		Vector v(3, 1);
		v(1) = sc->center[0];
		v(2) = sc->center[1];
		v(3) = sc->theta;

		sekf.step(u, v);
		cout << sekf.getX()(1) << " " << sekf.getX()(2) << endl;
		cout << "sc pos: " << sc->center[0] << endl;

		Vector X = sekf.getX();
		sc->center[0] = X(1);
		sc->center[1] = X(3);
		sc->theta = X(5);

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


