/*
 * StickController.h
 *
 *  Created on: Mar 18, 2015
 *      Author: robert
 */

#ifndef STICKCONTROLLER_H_
#define STICKCONTROLLER_H_

#include "GridController.h"


class StickController
{

public:
	StickController(GridController *gc);
	~StickController();
	void start();
	void stop();


    GridController * gc;
    bool _end;

    cv::Vec2d p0;
    cv::Vec2d p1;
    static const int len = 100;
    cv::Vec2d center;
    double theta;
    bool tracking;


protected:
	pthread_t _thread;    // Thread for tracking
};


#endif /* STICKCONTROLLER_H_ */
