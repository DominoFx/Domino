/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ISensor.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:35 AM
 */

#ifndef ISENSOR_H
#define ISENSOR_H

#include "enums.h"

class ISensor
{
public:
    ISensor(){};
    virtual ~ISensor(){};
    
    virtual float GetValue(Axis axis = Axis::Y) = 0;
};

#endif /* ISENSOR_H */

