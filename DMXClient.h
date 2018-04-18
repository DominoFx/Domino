#ifndef DMXCLIENT_H
#define DMXCLIENT_H

#include "../ClientNode/Client.h"
#include "dmx/DmxController.h"

class DMXClient: public Client {
public:
    DMXClient();
    virtual ~DMXClient();
    
    virtual bool Init();
    void Update();
    virtual void OnMessage(websocketpp::connection_hdl hdl, wsclient::message_ptr msg);
private:
    DmxController m_dmxController;
    std::mutex m_mutex;
};

#endif /* DMXCLIENT_H */

