#ifndef DMXCONTROLLER_H
#define DMXCONTROLLER_H

#include "enttecdmxusb.h"
#include <chrono>
#include <map>
#include <json/json.h>

class DmxController {
public:
    DmxController();

    bool Init();
    void Update();
    void Apply(const Json::Value& msgData);

    virtual ~DmxController();
private:
    void Set(int channel, int value, int speed = 0);
    void SetSpot(int spotIndex, int intensity, int red, int green, int blue);
    
    double realdata (int data);
    void getDefaultData();
    int capture(uint8_t sensorIndex, int capS,unsigned int freq);
    
    struct ChannelInterpolation
    {
        ChannelInterpolation():
        m_targetValue(-1)
        ,m_speed(0)
        ,m_currentValue(0)
        ,m_currentInterpolation(0)
        {
            
        };
        
        ChannelInterpolation(int target, int speed, int currentValue)
        {
            m_targetValue = target;
            m_speed = speed;
            m_currentValue = currentValue;
            
            m_currentInterpolation = currentValue;
        };
                
        int m_targetValue;
        int m_currentValue;
        int m_speed;
        double m_currentInterpolation;
    };
    
    struct ChannelDefinition
    {
        ChannelDefinition(int channelValue = -1, int maxValue = -1, int minValue = -1)
        {
            channel = channelValue;
            max = maxValue;
            min = minValue;
        }
        
        int channel;
        int max;
        int min;        
    };
    
    typedef std::map<int, ChannelInterpolation> Interpolations;
    typedef std::map<std::string, std::vector<ChannelDefinition> > IdChannelMap;
        
    Interpolations m_interpolations;
    EnttecDMXUSB* m_DMXInterface;
    bool m_initialized;
    std::chrono::time_point<std::chrono::system_clock> m_lastUpdate;
    bool m_continuousSend;
    float m_continuousDelay;
    std::chrono::time_point<std::chrono::system_clock> m_lastContinuousSend;
    
    IdChannelMap m_mappings;
    bool m_debugOutput;
    
    int m_presenceTransitionSpeed;
    float m_presenceThreshold;
    
    int m_maxValue;
    int m_value;
    bool m_direction;
    int m_fd;
    
    //std::vector<double> m_values;
	int m_sensorCount;
	bool m_useDmx;
        
        std::vector<int> m_previousValues;
};

#endif /* DMXCONTROLLER_H */

