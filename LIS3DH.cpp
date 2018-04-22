#include "LIS3DH.h"
#include <wiringPiI2C.h>

#define LIS3DH_DEFAULT_ADDRESS  (0x18)    // if SDO/SA0 is 3V, its 0x19

#define LIS3DH_REG_STATUS1       0x07
#define LIS3DH_REG_OUTADC1_L     0x08
#define LIS3DH_REG_OUTADC1_H     0x09
#define LIS3DH_REG_OUTADC2_L     0x0A
#define LIS3DH_REG_OUTADC2_H     0x0B
#define LIS3DH_REG_OUTADC3_L     0x0C
#define LIS3DH_REG_OUTADC3_H     0x0D
#define LIS3DH_REG_INTCOUNT      0x0E
#define LIS3DH_REG_WHOAMI        0x0F
#define LIS3DH_REG_TEMPCFG       0x1F
#define LIS3DH_REG_CTRL1         0x20
#define LIS3DH_REG_CTRL2         0x21
#define LIS3DH_REG_CTRL3         0x22
#define LIS3DH_REG_CTRL4         0x23
#define LIS3DH_REG_CTRL5         0x24
#define LIS3DH_REG_CTRL6         0x25
#define LIS3DH_REG_REFERENCE     0x26
#define LIS3DH_REG_STATUS2       0x27
#define LIS3DH_REG_OUT_X_L       0x28
#define LIS3DH_REG_OUT_X_H       0x29
#define LIS3DH_REG_OUT_Y_L       0x2A
#define LIS3DH_REG_OUT_Y_H       0x2B
#define LIS3DH_REG_OUT_Z_L       0x2C
#define LIS3DH_REG_OUT_Z_H       0x2D
#define LIS3DH_REG_FIFOCTRL      0x2E
#define LIS3DH_REG_FIFOSRC       0x2F
#define LIS3DH_REG_INT1CFG       0x30
#define LIS3DH_REG_INT1SRC       0x31
#define LIS3DH_REG_INT1THS       0x32
#define LIS3DH_REG_INT1DUR       0x33
#define LIS3DH_REG_CLICKCFG      0x38
#define LIS3DH_REG_CLICKSRC      0x39
#define LIS3DH_REG_CLICKTHS      0x3A
#define LIS3DH_REG_TIMELIMIT     0x3B
#define LIS3DH_REG_TIMELATENCY   0x3C
#define LIS3DH_REG_TIMEWINDOW    0x3D
#define LIS3DH_REG_ACTTHS        0x3E
#define LIS3DH_REG_ACTDUR        0x3F


LIS3DH::LIS3DH() {
}

LIS3DH::LIS3DH(const LIS3DH& orig) {
}

LIS3DH::~LIS3DH() {
}

bool LIS3DH::Read(int& x, int& y, int& z)
{
    int fd = wiringPiI2CSetup(LIS3DH_DEFAULT_ADDRESS);
    
    //Set hi rez mode 12 bits output
    //LIS3DH_REG_CTRL1 set to 0
    //LIS3DH_REG_CTRL4 set to 1
    
    if(fd < 0)
    {
        return false;
    }
    
    wiringPiI2CWriteReg8(fd, LIS3DH_DEFAULT_ADDRESS, LIS3DH_REG_OUT_X_L | 0x80);
    
    x  = wiringPiI2CReadReg8(fd, LIS3DH_REG_OUT_X_L);
    y  = wiringPiI2CReadReg8(fd, LIS3DH_REG_OUT_Y_L);
    z  = wiringPiI2CReadReg8(fd, LIS3DH_REG_OUT_Z_L);
    
    return true;
}
