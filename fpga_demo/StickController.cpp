/*
 * StickController.cpp
 *
 *  Created on: Mar 18, 2015
 *      Author: robert
 */




#include "StickController.h"
#include "GridController.h"
#include "AudioPlayer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include "StickEKF.h"

using std::cout;
using std::endl;
using cv::Vec2d;
using std::vector;
using std::queue;

StickController::StickController
(
    GridController *gc,
    AudioPlayer    *ap
)
: gc(gc), _ap(ap), _end(false), p0(600, 400), p1(800, 400), tracking(false), center(700, 400), theta(3.14159265 / 2.0),
  _xrange(0), _yrange(0), _anglerange(0), _time_limit_s(10), _last_update_time_s(0),
  _stick_color(0, 0, 255, 255)
{

    for(int i = -10; i <= 10; i += 5) {
        _xrange.push_back(i);
        _yrange.push_back(i);
    }

    for (double t = -3.14159 / 10.0; t < 3.14159 / 10.0; t += .2) {
        _anglerange.push_back(t);
    }
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

double costFunc(std::vector<GridSquare> closeSquares, Vec2d p0, Vec2d p1, Vec2d newCenter, Vec2d oldCenter,
		double newTheta, double oldTheta) {
	double distTerm = 0.0;
	int numActivated = 0;
	for (int i = 0; i < closeSquares.size(); i++) {
		GridSquare &gs = closeSquares[i];

		double x = gs._x0 + (double)gs._w / 2.0;
		double y = gs._y0 + (double)gs._h / 2.0;
		Vec2d p(x, y);
		if (gs.occupied && minimumDistance(p0, p1, p) < 20) {
			numActivated++;
			double dist = cv::norm(p - newCenter);
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
		double activatedTerm = 10.0 / numActivated;
		double distTerm = cv::norm(newCenter - oldCenter);
		double angleTerm = abs(newTheta - oldTheta);
//		double denom = activatedTerm + distTerm + angleTerm;
		cost = activatedTerm + angleTerm / 100.5 + distTerm / 4000.0;
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

		bool tracking = false;
		double minCost = std::numeric_limits<double>::max();

		Vec2d oldCenter = sc->center;
		double oldTheta = sc->theta;

        //for (int i = -10; i <= 10; i += 5) {
            //for (int j = -10; j <= 10; j += 5) {
//      for (int i=0; i < 3; i++) {
//          int newX = (i / 2) * (-1 * ((i % 2)) + 1) * 5;
//          for (int j=0; j < 3; j++) {
//              int newY = (j / 2) * (-1 * ((j % 2)) + 1) * 5;
                //for (double t = -3.14159 / 10.0; t < 3.14159 / 10.0; t += .2) {

		// shuffle the x, y, and angle values
	    sc->shuffle();

	    for (unsigned int xind = 0; xind < sc->_xrange.size(); ++xind)
	    {
            int i = sc->_xrange[xind];
	        for (unsigned int yind = 0; yind < sc->_yrange.size(); ++yind)
	        {
	            int j = sc->_yrange[yind];
	            for (unsigned int aind = 0; aind < sc->_anglerange.size(); ++aind)
	            {
	                double t = sc->_anglerange[aind];

					Vec2d offset(i, j);
					Vec2d newCenter = sc->center + offset;
					double newTheta = sc->theta + t;
					std::vector<Vec2d> pts = computeEndpoints(newCenter, newTheta, sc->len);
					if (pts[0][0] < 0 || pts[0][0] > sc->gc->_img_w || pts[0][1] < 0 || pts[0][1] > sc->gc->_img_h ||
							pts[1][0] < 0 || pts[1][0] > sc->gc->_img_w || pts[1][1] < 0 || pts[1][1] > sc->gc->_img_h) {
						continue;
					}

					double cost = costFunc(closeSquares, pts[0], pts[1], newCenter, oldCenter, newTheta, oldTheta);
					if (cost < minCost) {
					    tracking = true;
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

        // If cost meets threshold, the stick is activated
        cout << "min cost = " << minCost << endl;
        sc->tracking = tracking && minCost > 0.05;
        sc->updateStickState();

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


		usleep(500);
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


void StickController::shuffle() {
    std::random_shuffle(_xrange.begin(), _xrange.end());
    std::random_shuffle(_yrange.begin(), _yrange.end());
    std::random_shuffle(_anglerange.begin(), _anglerange.end());
}

void StickController::updateStickState() {

    // Get current time in seconds
    bool completed_spin = false;
    unsigned long long current_time_s = get_current_time_s_ll();
    cout << "current_time_s is " << current_time_s << endl;

    if (!tracking)
    {
        _stick_color = cv::Vec4d(0, 0, 255, 255);
    }

    // If atleast one second has gone by since last update, do update
    if (current_time_s != _last_update_time_s)
    {
        while (0 != _history_q.size())
        {
            Snapshot snapshot = _history_q.front();
            unsigned long long tdiff = current_time_s - snapshot.sec;
            if (tdiff > _time_limit_s)
            {
                _history_q.pop();
            }
            else
            {
                double angle_diff = abs(snapshot.angle - theta);
                if (angle_diff > 2*PI)
                {
                    // Completed 360 spin!
                    completed_spin = true;
                }
                break;
            }
        }

        if(completed_spin)
        {
            _stick_color = cv::Vec4i(255, 255, 255, 255);

            // If we compunsigned long long sec;leted a spin, clear history
            std::queue<Snapshot> empty_q;
            std::swap(_history_q, empty_q);
        }
        else
        {
            _stick_color = cv::Vec4i(0, 255, 255, 255);
        }

        // Add this snap shot of stick angle to the history queue
        Snapshot new_snapshot;
        new_snapshot.sec = current_time_s;
        new_snapshot.angle = theta;
        _history_q.push(new_snapshot);
        _last_update_time_s = current_time_s;
    }


}


