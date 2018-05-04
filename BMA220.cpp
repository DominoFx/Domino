/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   BMA220.cpp
 * Author: simon
 * 
 * Created on April 25, 2018, 12:19 AM
 */

#include "BMA220.h"
#include <wiringPiI2C.h>
#include <unistd.h>

#define BMA220_DEFAULT_ADDRESS 0xA

BMA220::BMA220() {
}

BMA220::~BMA220() {
}

float BMA220::GetValue(Axis axis /*= Axis::Y*/)
{
    int fd = wiringPiI2CSetup(BMA220_DEFAULT_ADDRESS);
    
    /*if (sensorIndex == 1)
    {
            //Filtering
            wiringPiI2CWriteReg8(fd, 0x20, 0x05);//can be set at"0x05""0x04"......"0x01""0x00", refer to Datasheet on wiki
                                                                                     //Sensitivity
            wiringPiI2CWriteReg8(fd, 0x22, 0x0);//can be set at"0x00""0x01""0x02""0x03", refer to Datasheet on wiki
    }*/

    int intData = 0;
    
    switch(axis)
    {
        case Axis::X:
        {
            intData = wiringPiI2CReadReg8(fd, 0x04); 
            	
            break;
        }
        
        case Axis::Y:
        {
            intData = wiringPiI2CReadReg8(fd, 0x06); 

            break;
        }
        
        case Axis::Z:
        {
            intData = wiringPiI2CReadReg8(fd, 0x08); 

            break;
        }
        
        default://Default on Y
        {
            intData = wiringPiI2CReadReg8(fd, 0x06); 

            break;
        }
    }
    
    close(fd);

    return Realdata(intData);   
}

double BMA220::Realdata (int data)
{
    double x,d;
    double nfactor = 1;	
    double slope = 0.0625;//  1.94/31;    sensitivity   +/- 2g range 
    int dat;

    data = data >> 2;
	
    if (data > 31)
    {          //MSB represents a negative number    
       dat = ~data + 1;      //2s compliment make it negative
       dat = dat & 0b011111; //bit mask anded to make positive 
       nfactor = -1;
    }
    else
    {
      dat = data; 
    }

    d =  (double)dat;  
    //x = d*slope*9.81*nfactor;            
    x = d*slope*nfactor;
    return x;  
}
