#ifndef DOMINOPLAYER_H
#define DOMINOPLAYER_H

#include "DominoState.h"
#include "OscController.h"
#include "enttecdmxusb.h"
//#include "ip/UdpSocket.h"
//#include "ip/PacketListener.h"

#include <thread>
#include <mutex>
// STL headers
#include <vector>

class DominoController;
class DominoSensorMaster;
class DominoSensorWorker;

typedef int DmxFrame[NB_CANAUX_MAX+1];



class DominoPlayerSequence
{
public:
    class Frame;
    typedef std::vector<Frame> FrameVec;
    typedef std::string String;

    class Frame
    {
    public:    
        friend class DominoPlayerSequence;
        typedef std::map<int,int> ValMap;
        typedef std::vector<int> ValBuf;
        
        Frame();
        float       GetTime() { return m_dmxTime; }
        int         GetDmxVal( int dmxChannel, int dmxCenterpoint )
        {
            int index = dmxChannel - (dmxCenterpoint - m_dmxChannelShift);
            return ((index<0) || (index>=(m_dmxValBuf.size()))?
                m_dmxValBase : m_dmxValBuf[index]);
        }
    protected:
        float       m_dmxTime;
        int         m_dmxChannelShift;
        int         m_dmxValBase;
        ValMap      m_dmxValMap;
        ValBuf      m_dmxValBuf;
    };
    
    DominoPlayerSequence( DominoController& context );
    int        Load( Json::Value& jsonRoot );
    int        GetId() { return m_id; }
    double     GetDuration() { return m_duration; }
    int        NumFrames();
    int        FindFrame( float dmxTime ); // index at or before the given time
    Frame*     GetFrame( int index ); // frame at given index
protected:
    DominoController& m_context;
    int        m_id;
    String     m_name;
    double     m_duration;
    FrameVec   m_dmxFrames;
};

//
// Class DominoPlayerAgent 
// Agent - Base class of Master and Worker
// 

class DominoPlayerAgent {
public:
    DominoPlayerAgent( DominoController& context );
    virtual ~DominoPlayerAgent();

    bool Init() {}

protected:
    DominoController& m_context;
    DominoParams& m_params;
};


//
// Class DominoPlayerMaster
// Master - Runs idle mode, transmits DMX to Workers
//

class DominoPlayerMaster : public DominoPlayerAgent {
public:
    class Task;
    typedef std::vector<Task> Queue;
    typedef DominoPlayerSequence Sequence;
    typedef std::map<int,DominoPlayerSequence*> SequencesMap;
    typedef std::vector<Json::Value*> SequencesJson;
    typedef DominoPlayerSequence::Frame Frame;
    typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;
    typedef std::chrono::duration<double> Duration;
    
    class Task
    {
    public:
        
        void       Run( Sequence* sequence, int dmxCenterpoint );
        bool       Running(); // checks current system time
        void       Update( DmxFrame& dmxout );  // checks current system time
    protected:
        Sequence*  m_sequence;
        int        m_dmxCenterpoint;
        TimePoint  m_start;
    };
    

    DominoPlayerMaster( DominoController& context );
    virtual ~DominoPlayerMaster();

    bool Init();
    void Update();
    
    bool RegisterWorker( const std::string& workerAddress, int workerPort, int workerIndex );

    bool LoadSequences( const char* filename ); // config sequences file
    bool LoadSequence( const char* filename );  // single sequence file
    bool PlaySequence( int id, int dmxCenterpoint );
    Sequence* FindSequence( int id );

protected:  
    bool m_initialized;
    
    // Channel values for DMX
    std::mutex m_mutex;
    DmxFrame m_dmxout;
    
    // Thread to play sequences for DMX
    std::thread m_dmxThread;
    float m_dmxDelay;
    TimePoint m_lastUpdate;
    Queue m_queue;
    
    // Sequences for DMX
    SequencesMap m_sequencesMap;
    SequencesJson m_sequencesJson;

    // Listener object, receives DMX play commands from OSC
    //OscListener m_oscListener;
    
    std::vector<OscController*> m_workers;

    // Local methods
    //int StartOscThread();
    void ProcessIdleSound( const char *data, int size, const IpEndpointName& remoteEndpoint );
    static void ProcessIdleSound( const char *data, int size, const IpEndpointName& remoteEndpoint, void* param );
    int StartDmxThread();
    void DmxThread();
    static void DmxThreadBootstrap( DominoPlayerMaster* parent ) {parent->DmxThread();}
};


//
// Class DominoPlayerWorker
// Worker - Runs interactive mode, receives DMX from Master
//

class DominoPlayerWorker : public DominoPlayerAgent {
public:
    DominoPlayerWorker( DominoController& context );
    virtual ~DominoPlayerWorker();

    bool Init();
    void Update( DmxFrame& dmxout );
    
    float GetFPS() {return m_dmxFps;}
    
protected:
    bool m_initialized;

    // Channel values for DMX
    std::mutex m_mutex;
    DmxFrame m_dmxout;
    DmxFrame m_dmxoutSensor;
    DmxFrame m_dmxoutPlayer;
    
    // Thread to post DMX frames to hardware
    std::thread m_dmxThread;
    EnttecDMXUSB* m_dmxInterface;
    std::chrono::time_point<std::chrono::system_clock> m_dmxLastUpdate;
    float m_dmxFps;
    
    // Listener object, receives DMX frames from master
    //OscListener m_oscListener;

    // Local methods
    //int StartOscThread();
    void ProcessIdleDmx( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint );
    static void ProcessIdleDmx( const char *argsData, int argsSize, const IpEndpointName& remoteEndpoint, void* param );
    int StartOutputDmxThread();
    void OutputDmxThread();
    static void OutputDmxThreadBootstrap( DominoPlayerWorker* parent ) {parent->OutputDmxThread();}
};

#endif /* DOMINOPLAYER_H */

