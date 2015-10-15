/*
 * StickController.h
 *
 *  Created on: Mar 18, 2015
 *      Author: robert
 */

#ifndef STICKCONTROLLER_H_
#define STICKCONTROLLER_H_

#include "GridController.h"
#include <queue>
#include "StickEKF.h"


struct Snapshot
{
    double             angle;
    unsigned long long sec;
};


_INTERFACE_ StickControllerListener
{
public:
    virtual void handleSpinCompleted() = 0;
};


class StickController
{

public:
	StickController(GridController *gc);
	~StickController();
	void start();
	void stop();
	void shuffle();
	void updateStickState();
	void reset();
    void registerListener(StickControllerListener * obj);
    void notifyListeners(void);

    GridController * gc;

    bool _end;

	StickEKF sekf;
    cv::Vec2d p0;
    cv::Vec2d p1;
    static const int len = 80;
    static const int proximity = 150;
    cv::Vec2d center;
    double theta;
    bool tracking;

    long nonTrackingTime;

    std::vector<int>    _xrange;
    std::vector<int>    _yrange;
    std::vector<double> _anglerange;

    unsigned long long    _time_limit_s;
    unsigned long long    _last_update_time_s;

    cv::Vec4i             _stick_color;

    std::queue<Snapshot>  _history_q;
    std::vector<StickControllerListener *> _listeners;

protected:
	pthread_t _thread;    // Thread for tracking
};


#endif /* STICKCONTROLLER_H_ */
