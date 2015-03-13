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
			double x = sq._x0 + (double) sq._w / 2.0;
			double y = sq._y0 + (double) sq._h / 2.0;
			double dist = std::sqrt((ballController->xPos - x) * (ballController->xPos - x)
					+ (ballController->yPos - y) * (ballController->yPos - y));
			if (dist < ballController->radius) {
				ballController->hit = true;
				sX += x;
				sY += y;
				numHits++;
			}
		}

		if (ballController->hitTimer > 0) {
			ballController->hitTimer--;
		}

		if (ballController->hit == true && ballController->hitTimer <= 0) {
			ballController->hitTimer = 50;
			sX /= (double) numHits;
			sY /= (double) numHits;
			sX = ballController->xPos - sX;
			sY = ballController->yPos - sY;
			double sLen = std::sqrt(sX * sX + sY * sY);
			sX = sX / sLen;
			sY = sY / sLen;
			double newVX = sX - ballController->vX;
			double newVY = sY - ballController->vY;
			cout << "x: " << newVX << endl;
			cout << "Y: " << newVY << endl;
			double len = std::sqrt(newVX * newVX + newVY * newVY);
			ballController->vX = 2.0 * (newVX / len);
			ballController->vY = 2.0 * (newVY / len);

//			double vXN = 0;
//			double vYN = 0;
//			double len = std::sqrt(ballController->vX * ballController->vX + ballController->vY * ballController->vY);
//			vXN = ballController->vX / len;
//			vYN = ballController->vY / len;
//
//			double sLen = std::sqrt(sX * sX + sY * sY);
//			sX = sX / sLen;
//			sY = sY / sLen;
//
//			double theta = -1.0 * (vXN * sX + vYN * sY);
//			ballController->vX += theta * 2.0;
//			ballController->vY += theta * 2.0;
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
