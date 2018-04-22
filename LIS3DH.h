#ifndef LIS3DH_H
#define LIS3DH_H

class LIS3DH {
public:
    LIS3DH();
    LIS3DH(const LIS3DH& orig);
    virtual ~LIS3DH();
    
    enum Range
    {
      LIS3DH_RANGE_16_G         = 0b11,   // +/- 16g
      LIS3DH_RANGE_8_G           = 0b10,   // +/- 8g
      LIS3DH_RANGE_4_G           = 0b01,   // +/- 4g
      LIS3DH_RANGE_2_G           = 0b00    // +/- 2g (default value)
    };

    enum Axis
    {
      LIS3DH_AXIS_X         = 0x0,
      LIS3DH_AXIS_Y         = 0x1,
      LIS3DH_AXIS_Z         = 0x2,
    };
    
    enum DataRate
    {
      LIS3DH_DATARATE_400_HZ     = 0b0111, //  400Hz 
      LIS3DH_DATARATE_200_HZ     = 0b0110, //  200Hz
      LIS3DH_DATARATE_100_HZ     = 0b0101, //  100Hz
      LIS3DH_DATARATE_50_HZ      = 0b0100, //   50Hz
      LIS3DH_DATARATE_25_HZ      = 0b0011, //   25Hz
      LIS3DH_DATARATE_10_HZ      = 0b0010, // 10 Hz
      LIS3DH_DATARATE_1_HZ       = 0b0001, // 1 Hz
      LIS3DH_DATARATE_POWERDOWN  = 0,
      LIS3DH_DATARATE_LOWPOWER_1K6HZ  = 0b1000,
      LIS3DH_DATARATE_LOWPOWER_5KHZ  =  0b1001,

    };
    
    static bool Read(int& x, int& y, int& z);
private:

};

#endif /* LIS3DH_H */

