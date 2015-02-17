/*
 * BallController.h
 *
 *  Created on: Feb 12, 2015
 *      Author: robert
 */

#ifndef BALLCONTROLLER_H_
#define BALLCONTROLLER_H_

#include "Public.h"
#include "GridController.h"

class BallController
{
private:
	pthread_t thread;

public:
	BallController(GridController *gc, int imgWidth, int imgHeight, int radius);
	~BallController();
	void start();
	void stop();

	int xPos;
	int yPos;
	int radius;
	bool hit;
	GridController *gc;
	int imgWidth;
	int imgHeight;
};




#endif /* BALLCONTROLLER_H_ */
