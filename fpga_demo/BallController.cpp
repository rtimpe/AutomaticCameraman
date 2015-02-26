/*
 * BallController.cpp
 *
 *  Created on: Feb 12, 2015
 *      Author: robert
 */

#include "BallController.h"


BallController::BallController(GridController *gc, int imgWidth, int imgHeight, int radius) :
gc(gc), imgWidth(imgWidth), imgHeight(imgHeight), xPos(imgWidth / 2), yPos(imgHeight / 2),
radius(radius), hit(false), vX(1.0), vY(-.5)
{ }

void* ballFunc(void *arg) {
	BallController *ballController = (BallController *)arg;

	while (true) {
		ballController->hit = false;
		double sX = 0;
		double sY = 0;
		int numHits = 0;
		for (int i = 0; i < ballController->gc->_squares.size(); i++) {
			GridSquare sq = ballController->gc->_squares[i];
			if (!sq.occupied) {
				continue;
			}
			double dist = std::sqrt((ballController->xPos - (double)sq._x0) * (ballController->xPos - (double)sq._x0)
					+ (ballController->yPos - (double)sq._y0) * (ballController->yPos - (double)sq._y0));
			if (dist < ballController->radius) {
				ballController->hit = true;
				sX += sq._x0;
				sY += sq._y0;
				numHits++;
			}
		}

		if (ballController->hit == true) {
			sX /= (double) numHits;
			sY /= (double) numHits;
			double vXN = 0;
			double vYN = 0;
			double len = std::sqrt(ballController->vX * ballController->vX + ballController->vY * ballController->vY);
			vXN = ballController->vX / len;
			vYN = ballController->vY / len;

			double sLen = std::sqrt(sX * sX + sY * sY);
			sX = sX / sLen;
			sY = sY / sLen;

			double theta = -1.0 * (vXN * sX + vYN * sY);
			ballController->vX += theta * 2.0;
			ballController->vY += theta * 2.0;
		}

		ballController->xPos += ballController->vX;
		ballController->yPos += ballController->vY;

		if (ballController->xPos < 0) {
			ballController->xPos = 0;
			ballController->vX *= -1.0;
		}
		if (ballController->xPos > ballController->imgWidth) {
			ballController->xPos = ballController->imgWidth;
			ballController->vX *= -1.0;
		}
		if (ballController->yPos < 0) {
			ballController->yPos = 0;
			ballController->vY *= -1.0;
		}
		if (ballController->yPos > ballController->imgHeight) {
			ballController->yPos = ballController->imgHeight;
			ballController->vY *= -1.0;
		}

//		std::cout << "xpos: " << ballController->xPos << " ypos: " << ballController->yPos << std::endl;
//		std::cout << "x: " << ballController->vX << " y: " << ballController->vY << std::endl;
		usleep(10000);
	}
}

void BallController::start() {
	pthread_create(&thread, NULL, ballFunc, this);
}

void BallController::stop() {

}
