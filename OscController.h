#ifndef OSCCONTROLLER_H
#define OSCCONTROLLER_H

#include "osc/OscOutboundPacketStream.h"
#include "ip/UdpSocket.h"
#include "ip/PacketListener.h"
#include <string>
#include <vector>
#include <map>
#include <thread>

#define SAFE_DELETE(x) if((x)!=nullptr) {delete (x); (x)=nullptr;}


#define OSC_OUTPUT_BUFFER_SIZE 1024


//
// Class OscController
// 

class OscController {
public:
    OscController();
    virtual ~OscController();
    bool Open(std::string address = "127.0.0.1", int port = 8888);
    bool Opened();
    int Port();
    std::string Address();

    // Send data with params
    void Send(std::string& tag, int paramCount, float* paramBuf );
    void Send(std::string& tag, int paramCount, int* paramBuf );
    
    // Send data with integer param and additional params
    void Send(std::string& tag, int param0, int paramCountRest, const char** paramBufRest );
    void Send(std::string& tag, int param0, int param1, int paramCountRest, const char** paramBufRest );
    void Send(std::string& tag, int param0, int paramCountRest, float* paramBuf );
    
private:
    IpEndpointName m_ipEndpointName;
    
    UdpSocket* m_transmitSocket;
    char m_outputBuffer[OSC_OUTPUT_BUFFER_SIZE];
};


//
// Class OscBroadcaster
// 

class OscBroadcaster {
public:
    OscBroadcaster();
    virtual ~OscBroadcaster();
    bool Open(int port = 8888);
    bool Opened();
    bool IsInitialized();

    // Send data with params
    void Send(std::string& tag, int paramCount, int* paramBuf );
    
private:
    UdpBroadcastSocket* m_broadcastSocket;
    char m_outputBuffer[OSC_OUTPUT_BUFFER_SIZE];
};



//
// Class OscListener
// 

typedef void (*OscCallback)( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );

class OscListener : public PacketListener {
public:
    typedef struct
    {
        std::string tagStr;
        int tagLength;
        OscCallback func;
        void* param;
    } OscFunc;
    
    typedef struct
    {
        bool operator()(const char *first, const char  *second) { return strcmp(first, second) < 0; }
    } OscComparator;
    
    // Map from tags to client data.
    // Key is char* instead of std::string, so map lookups do not require memory allocation
    typedef std::map<const char*,OscFunc,OscComparator> ClientMap;    
    typedef std::vector<char*> TagVec;

    OscListener( const char* debugName );
    virtual ~OscListener();
    
    int Register( const char* tag, OscCallback callbackFunc, void* callbackParam );
    int Unregister( OscCallback callbackFunc );
    int Open( int port );
    bool Opened();
    int Port();
    
private:
    ClientMap m_clientMap;
    TagVec m_tagVec;
    std::string m_debugName;
    int m_port;
    UdpSocket* m_receiveSocket;
    SocketReceiveMultiplexer* m_receiveMultiplexer;
    std::thread m_receiveThread;
    void ReceiveThread();
    static void ReceiveThreadBootstrap( OscListener* parent ) { parent->ReceiveThread(); }
    // from PacketListener
    void ProcessPacket( const char *data, int size, const IpEndpointName& remoteEndpoint );
};


#endif /* OSCCONTROLLER_H */
