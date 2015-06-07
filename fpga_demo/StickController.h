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

class AudioPlayer;

struct Snapshot
{
    double             angle;
    unsigned long long sec;
};


class StickController
{

public:
	StickController(GridController *gc, AudioPlayer * ap);
	~StickController();
	void start();
	void stop();
	void shuffle();
	void updateStickState();


    GridController * gc;
    AudioPlayer * _ap;
    bool _end;

    cv::Vec2d p0;
    cv::Vec2d p1;
    static const int len = 100;
    cv::Vec2d center;
    double theta;
    bool tracking;

    std::vector<int>    _xrange;
    std::vector<int>    _yrange;
    std::vector<double> _anglerange;

    unsigned long long    _time_limit_s;
    unsigned long long    _last_update_time_s;

    cv::Vec4i             _stick_color;

    std::queue<Snapshot>  _history_q;

protected:
	pthread_t _thread;    // Thread for tracking
};


#endif /* STICKCONTROLLER_H_ */
