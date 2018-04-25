/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BMA220.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:19 AM
 */

#ifndef BMA220_H
#define BMA220_H

#include "ISensor.h"

class BMA220: public ISensor {
public:
    BMA220();
    virtual ~BMA220();
    
    float GetValue(Axis axis = Axis::Y);
private:
    double Realdata (int data);
};

#endif /* BMA220_H */

