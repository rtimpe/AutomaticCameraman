/*
 * StickKF.h
 *
 *  Created on: Apr 27, 2015
 *      Author: robert
 */

#ifndef STICKEKF_H_
#define STICKEKF_H_

#include "kalman/ekfilter.hpp"

class StickEKF : public Kalman::EKFilter<double, 1> {
public:
        StickEKF();

protected:

        void makeA();
        void makeH();
        void makeV();
        void makeR();
        void makeW();
        void makeQ();
        void makeProcess();
        void makeMeasure();

        double period, mass, drag;
};

typedef StickEKF::Vector KVector;
typedef StickEKF::Matrix KMatrix;

#endif /* STICKEKF_H_ */
