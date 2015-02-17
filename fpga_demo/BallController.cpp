/*
 * BallController.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: robert
 */

#include "BallController.h"


BallController::BallController(GridController *gc, int imgWidth, int imgHeight, int radius) :
gc(gc), imgWidth(imgWidth), imgHeight(imgHeight), xPos(imgWidth / 2), yPos(imgHeight / 2),
radius(radius), hit(false)
{ }

void* ballFunc(void *arg) {
	BallController *ballController = (BallController *)arg;

	while (true) {
		ballController->hit = false;
		for (int i = 0; i < ballController->gc->_squares.size(); i++) {
			GridSquare sq = ballController->gc->_squares[i];
			if (!sq.occupied) {
				continue;
			}
			double dist = std::sqrt(((double)ballController->xPos - (double)sq._x0) * ((double)ballController->xPos - (double)sq._x0)
					+ ((double)ballController->yPos - (double)sq._y0) * ((double)ballController->yPos - (double)sq._y0));
			if (dist < ballController->radius) {
				ballController->hit = true;
			}
		}

		usleep(10000);
	}
}

void BallController::start() {
	pthread_create(&thread, NULL, ballFunc, this);
}

void BallController::stop() {

}
