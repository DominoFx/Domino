/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: simon
 *
 * Created on April 25, 2018, 12:45 AM
 */

#ifndef COMMON_H
#define COMMON_H

enum Axis
{
    X = 1,
    Y = 2,
    Z = 4,
    XYZ = 7
};

//
// Data globals
//

extern int signal_shutdown; // signals threads to stop


#endif /* ENUMS_H */
