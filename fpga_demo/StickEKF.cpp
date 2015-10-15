/*
 * StickEKF.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: robert
 */

#include "StickEKF.h"
#include <cmath>
#include <iostream>

typedef StickEKF::Vector KVector;
typedef StickEKF::Matrix KMatrix;

StickEKF::StickEKF()
{
	setDim(6, 0, 3, 3, 3);
	period = 0.02;
	drag = 5;
	mass = 100;
}


void StickEKF::makeA() {
	A(1,1) = 1.0;
	A(1,2) = period;
	A(1,3) = 0.0;
	A(1,4) = 0.0;
	A(1,5) = 0.0;
	A(1,6) = 0.0;

	A(2,1) = 0.0;
	A(2,2) = 1.0;
	A(2,3) = 0.0;
	A(2,4) = 0.0;
	A(2,5) = 0.0;
	A(2,6) = 0.0;

	A(3,1) = 0.0;
	A(3,2) = 0.0;
	A(3,3) = 1.0;
	A(3,4) = period;
	A(3,5) = 0.0;
	A(3,6) = 0.0;

	A(4,1) = 0.0;
	A(4,2) = 0.0;
	A(4,3) = 0.0;
	A(4,4) = 1.0;
	A(4,5) = 0.0;
	A(4,6) = 0.0;

	A(5,1) = 0.0;
	A(5,2) = 0.0;
	A(5,3) = 0.0;
	A(5,4) = 0.0;
	A(5,5) = 1.0;
	A(5,6) = period - period*period*drag/mass;

	A(6,1) = 0.0;
	A(6,2) = 0.0;
	A(6,3) = 0.0;
	A(6,4) = 0.0;
	A(6,5) = 0.0;
	A(6,6) = 1.0 - 2 * period*drag/mass;
}
void StickEKF::makeH() {
	H(1,1) = 1.0;
	H(1,2) = 0.0;
	H(1,3) = 0.0;
	H(1,4) = 0.0;
	H(1,5) = 0.0;
	H(1,6) = 0.0;

	H(2,1) = 0.0;
	H(2,2) = 0.0;
	H(2,3) = 1.0;
	H(2,4) = 0.0;
	H(2,5) = 0.0;
	H(2,6) = 0.0;

	H(3,1) = 0.0;
	H(3,2) = 0.0;
	H(3,3) = 0.0;
	H(3,4) = 0.0;
	H(3,5) = 1.0;
	H(3,6) = 0.0;
}
void StickEKF::makeV() {
	V(1,1) = 1.0;
	V(1,2) = 0.0;
	V(1,3) = 0.0;

	V(2,1) = 0.0;
	V(2,2) = 1.0;
	V(2,3) = 0.0;

	V(3,1) = 0.0;
	V(3,2) = 0.0;
	V(3,3) = 1.0;
}
void StickEKF::makeR() {
	R(1,1) = 0.1;
	R(1,2) = 0.0;
	R(1,3) = 0.0;

	R(2,1) = 0.0;
	R(2,2) = 0.1;
	R(2,3) = 0.0;

	R(3,1) = 0.0;
	R(3,2) = 0.0;
	R(3,3) = 15.0;
}
void StickEKF::makeW() {
	W(1,1) = 0.0;
	W(1,2) = 0.0;
	W(1,3) = 0.0;

	W(2,1) = 1.0;
	W(2,2) = 0.0;
	W(2,3) = 0.0;

	W(3,1) = 0.0;
	W(3,2) = 0.0;
	W(3,3) = 0.0;

	W(4,1) = 0.0;
	W(4,2) = 1.0;
	W(4,3) = 0.0;

	W(5,1) = 0.0;
	W(5,2) = 0.0;
	W(5,3) = 0.0;

	W(6,1) = 0.0;
	W(6,2) = 0.0;
	W(6,3) = 1.0;
}
void StickEKF::makeQ() {
	Q(1,1) = 0.1;
	Q(1,2) = 0.0;
	Q(1,3) = 0.0;


	Q(2,1) = 0.0;
	Q(2,2) = 0.1;
	Q(2,3) = 0.0;

	Q(3,1) = 0.0;
	Q(3,2) = 0.0;
	Q(3,3) = 0.1;
}
void StickEKF::makeProcess() {
	KVector x_(x.size());
	x_(1) = x(1) + x(2)*period;
	x_(2) = x(2);
	x_(3) = x(3) + x(4)*period;
	x_(4) = x(4);
	x_(5) = x(5) + x(6)*period - period*period*drag*x(6) / (2 * mass);
	x_(6) = x(6) - period*drag*x(6)/mass;

	if (x_(6) > 1.0) {
		x_(6) = 1.0;
	}
	if (x_(1) < 0) {
		x_(1) = 0;
		x_(2) = 0;
	}
	if (x_(1) > 1280) {
		x_(1) = 1280;
		x_(2) = 0;
	}
	if (x_(3) < 0) {
		x_(3) = 0;
		x_(4) = 0;
	}
	if (x_(3) > 720) {
		x_(3) = 720;
		x_(4) = 0;
	}

	if (x_(2) > 20.0) {
		x_(2) = 20.0;
	}
	if (x_(2) < -20.0) {
		x_(2) = -20.0;
	}
	if (x_(4) > 20.0) {
		x_(4) = 20.0;
	}
	if (x_(4) < -20.0) {
		x_(4) = -20.0;
	}
	x.swap(x_);
}
void StickEKF::makeMeasure() {
	z(1)=x(1);
	z(2)=x(3);
	z(3)=x(5);
}



