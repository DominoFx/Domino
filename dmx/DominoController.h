#ifndef DMXCONTROLLER_H
#define DMXCONTROLLER_H

#include "enttecdmxusb.h"
#include <chrono>
#include <map>
#include <json/json.h>
#include "OscController.h"
#include "ISensor.h"
#include "enums.h"

class DominoController {
public:
    DominoController();

    bool Init();
    void Update();
    void Apply(const Json::Value& msgData);

    virtual ~DominoController();
private:
  
    
    double realdata (int data);
    bool capture(uint8_t sensorIndex);
          
    EnttecDMXUSB* m_DMXInterface;
    bool m_initialized;
    std::chrono::time_point<std::chrono::system_clock> m_lastUpdate;
    bool m_continuousSend;
    float m_continuousDelay;
    std::chrono::time_point<std::chrono::system_clock> m_lastContinuousSend;
    
    bool m_debugOutput;
        
    int m_fd;//wiringPiFd
    
    int m_sensorCount;
    bool m_useDmx;
    
    std::vector<double> m_values   ; 
    std::vector<int> m_previousDMXValues;
    
    std::string m_oscTag;
    OscController m_oscController;
    int m_oscDMXSendThreshold;
    Axis m_axis;//0 = x, 1 = y, z = 2
    float m_normalizationValue;
    ISensor* m_sensor;
    int m_previousSentDMX;
};

#endif /* DMXCONTROLLER_H */

