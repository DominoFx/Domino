#include "zonemanager.h"

zonemanager::zonemanager()
{
    zone newZone;
    newZone.m_id = "1";
    newZone.m_position = CvPoint(256, 212);
    newZone.m_innerRadius = 50;
    newZone.m_outerRadius = 100;
    
    m_zones.push_back(newZone);
}

void zonemanager::Update(const depthpeoplecounter::PeopleList& peoples, Mat& resultImage, client::connection_ptr connection)
{    
    if(peoples.size() == 0)
    {
        return;
    }
    
    for(Zones::iterator zoneIt = m_zones.begin(); zoneIt != m_zones.end(); ++zoneIt)
    {
        cv::circle(resultImage, zoneIt->m_position, zoneIt->m_innerRadius, cvScalar(255, 255, 255));
        cv::circle(resultImage, zoneIt->m_position, zoneIt->m_outerRadius, cvScalar(255, 255, 255));    

        //Find closest person position for each zone
        depthpeoplecounter::PeopleList::const_iterator itClosest = peoples.begin();
        float closestDistance = std::numeric_limits<float>::max();
        
        for(depthpeoplecounter::PeopleList::const_iterator it = peoples.begin(); it != peoples.end(); ++it)
        {
            float distance = Distance(zoneIt->m_position, it->position);
            
            if(distance < closestDistance)
            {
                closestDistance = distance;
                itClosest = it;
            }
        }
        
        float ratio = zoneIt->GetZoneRatio(itClosest->position);

        std::ostringstream ss;
        ss << ratio;
        std::string ratioStr(ss.str());

        std::string message;
        message = "{'data':[ {'presence':";
        message += ratioStr;
        message += "}]}";
        
        if(connection)
        {
            //connection->send("zone=" + zoneIt->m_id + "&presence=" + ratioStr, websocketpp::frame::opcode::TEXT);
            connection->send(message, websocketpp::frame::opcode::TEXT);
        }
    }
}

double zonemanager::Distance(const CvPoint& point1, const CvPoint& point2)
{
    return sqrt(pow((point2.x - point1.x), 2) + pow((point2.y - point1.y), 2));
}

zonemanager::~zonemanager() {
}

