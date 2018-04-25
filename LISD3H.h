/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LISD3H.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:56 AM
 */

#ifndef LISD3H_H
#define LISD3H_H

#include "ISensor.h"

class LISD3H: public ISensor {
public:
    LISD3H();
    virtual ~LISD3H();
    
    float GetValue(Axis axis = Axis::Y);
private:

};

#endif /* LISD3H_H */

