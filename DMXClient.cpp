#include "DMXClient.h"

DMXClient::DMXClient() {
}

DMXClient::~DMXClient() {
}

bool DMXClient::Init()
{   //TODO: use configuration    
    if(Client::Init())
    {
        return m_dmxController.Init();
    }
    
    return false;
}

void DMXClient::Update()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dmxController.Update();
}

void DMXClient::OnMessage(websocketpp::connection_hdl hdl, wsclient::message_ptr msg)
{
    Client::OnMessage(hdl, msg);
 
    std::lock_guard<std::mutex> lock(m_mutex);
    if(msg->get_opcode() == websocketpp::frame::opcode::TEXT)
    {
        Json::Value jsonMessage = ParseMessage(msg);
        
        if(jsonMessage.isMember("data"))
        {            
            Json::Value msgData = jsonMessage.get("data", jsonMessage);
            
            if(!msgData.isArray())
            {
                return;
            }
                        
            if(msgData.size() > 0)
            { 
                m_dmxController.Apply(msgData);
            }
        }
    }
}