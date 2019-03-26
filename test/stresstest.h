// stresstest.h

#ifndef __STRESSTEST_H
#define __STRESSTEST_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "blockhead/type.h"
#include "network/networkservice.h"
#include "dnseed/netproto.h"
#include "dnseed/netpeer.h"
#include "dnseed/addrpool.h"
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>


namespace stresstest
{

using namespace std;
using namespace network;
using namespace blockhead;
using namespace dnseed;


class CStressTestConfig
{
public:
    CStressTestConfig();
    ~CStressTestConfig();

    bool ReadConfig(int argc, char *argv[], 
        const boost::filesystem::path& pathDefault, const std::string& strConfile);
    void ListConfig();
    void Help();

private:
    bool TokenizeConfile(const char *pzConfile,vector<string>& tokens);
    void AddToken(string& sIn, vector<string>& tokens);
    static std::pair<std::string,std::string> ExtraParser(const std::string& s);

public:
    CNetConfig tNetCfg;

    bool fHelp;
    bool fTestNet;
    bool fDebug;
    unsigned int nMagicNum;
    string sWorkDir;

    uint16 nTrustPort;
    string sLocalIpaddr;
    int iLocalIpType;
    uint16 nListenBeginPort;
    uint16 nListenPortCount;
    uint32 nPerPortCount;

    string sConnectIpaddr;
    uint16 nConnectPort;
    uint32 nPerConnectCount;
    uint32 nMaxConnectCount;

    bool fRecvTrustConnect;

    boost::filesystem::path pathRoot;
    boost::filesystem::path pathData;
    boost::filesystem::path pathConfile;

    boost::program_options::options_description defaultDesc;
};

class CStressTestStat
{
public:
    CStressTestStat()
    {
        nTotalInConnectCount = 0;
        nTotalOutConnectCount = 0;
        nTotalOutSuccessCount = 0;
        nTotalOutFailCount = 0;

        nCurConnectCount = 0;
        nCurInBoundCount = 0;
        nCurOutBoundCount = 0;
    }
    ~CStressTestStat() {}

    CStressTestStat& operator=(CStressTestStat& t)
    {
        nTotalInConnectCount = t.nTotalInConnectCount;
        nTotalOutConnectCount = t.nTotalOutConnectCount;
        nTotalOutSuccessCount = t.nTotalOutSuccessCount;
        nTotalOutFailCount = t.nTotalOutFailCount;

        nCurConnectCount = t.nCurConnectCount;
        nCurInBoundCount = t.nCurInBoundCount;
        nCurOutBoundCount = t.nCurOutBoundCount;
        return *this;
    }

    CStressTestStat& operator+=(CStressTestStat& t)
    {
        nTotalInConnectCount += t.nTotalInConnectCount;
        nTotalOutConnectCount += t.nTotalOutConnectCount;
        nTotalOutSuccessCount += t.nTotalOutSuccessCount;
        nTotalOutFailCount += t.nTotalOutFailCount;

        nCurConnectCount += t.nCurConnectCount;
        nCurInBoundCount += t.nCurInBoundCount;
        nCurOutBoundCount += t.nCurOutBoundCount;
        return *this;
    }

public:
    uint32 nTotalInConnectCount;
    uint32 nTotalOutConnectCount;
    uint32 nTotalOutSuccessCount;
    uint32 nTotalOutFailCount;

    uint32 nCurConnectCount;
    uint32 nCurInBoundCount;
    uint32 nCurOutBoundCount;
};



class CStressTestMsgWorkThread;
class CStressTestNetPeer;
class CStressTestBbAddrPool;

class CStressTestWorkThreadPool
{
public:
    CStressTestWorkThreadPool(CStressTestConfig *pCfg, CNetWorkService *nws, CStressTestBbAddrPool *pBbAddrPoolIn);
    ~CStressTestWorkThreadPool();

    bool StartAll();
    void StopAll();

    bool StatData(CStressTestStat& tStat);

private:
    CStressTestMsgWorkThread *pWorkThreadTable[MAX_NET_WORK_THREAD_COUNT];
    uint32 nWorkThreadCount;

    CStressTestConfig *pTestCfg;
    CNetWorkService *pNetWorkService;
    CStressTestBbAddrPool *pBbAddrPool;
};


class CStressTestMsgWorkThread
{
    friend class CStressTestNetPeer;
public:
    CStressTestMsgWorkThread(uint32 nThreadCount, uint32 nThreadIndex, CStressTestConfig *pCfg, CNetWorkService *nws, CStressTestBbAddrPool *pBbAddrPoolIn);
    ~CStressTestMsgWorkThread();

    bool Start();
    void Stop();

    bool GetStatData(CStressTestStat& tStat);

private:
    void Work();
    void Timer();

    void StressTestDoPacket(CMthNetPackData *pPackData);
    bool StressTestSendDataPacket(uint64 nNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, char *pBuf, uint32 nLen);

    CStressTestNetPeer* StressTestAddNetPeer(bool fInBoundIn, uint64 nPeerNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn);
    void StressTestDelNetPeer(uint64 nPeerNetId);
    CStressTestNetPeer* StressTestGetNetPeer(uint64 nPeerNetId);

    void StressTestActiveClosePeer(uint64 nPeerNetId);
    bool StressTestHandlePeerHandshaked(CStressTestNetPeer *pPeer);

    void StressTestDoCallConnect();
    void StressTestDoPeerState(time_t tmCurTime);

    bool StressTestStartConnectPeer(CBbAddr& tBbAddr);

private:
    bool fRunFlag;
    boost::thread *pThreadMsgWork;

    uint32 nWorkThreadCount;
    uint32 nWorkThreadIndex;
    CStressTestConfig *pTestCfg;
    CNetWorkService *pNetWorkService;
    CNetDataQueue *pNetDataQueue;
    CStressTestBbAddrPool *pBbAddrPool;
    uint32 nPersCalloutAddrCount;

    int64 nStartTestTime;
    uint32 nCompTestCount;

    map<uint64,CStressTestNetPeer*> mapPeer;

    time_t tmPrevTimerDoTime;

    CStressTestStat tNsStatData;
    boost::shared_mutex lockStat;
};



typedef enum _DNP_E_ST_PEER_STATE
{
    DNP_E_ST_PEER_STATE_INIT,

    DNP_E_ST_PEER_STATE_IN_CONNECTED,
    DNP_E_ST_PEER_STATE_IN_WAIT_HELLO_ACK,
    DNP_E_ST_PEER_STATE_IN_HANDSHAKED_COMPLETE,
    DNP_E_ST_PEER_STATE_IN_WAIT_ADDRESS_RSP,
    DNP_E_ST_PEER_STATE_IN_COMPLETE,

    DNP_E_ST_PEER_STATE_OUT_CONNECTING,
    DNP_E_ST_PEER_STATE_OUT_CONNECTED,
    DNP_E_ST_PEER_STATE_OUT_WAIT_HELLO,
    DNP_E_ST_PEER_STATE_OUT_HANDSHAKED_COMPLETE,
    DNP_E_ST_PEER_STATE_OUT_WAIT_ADDRESS_RSP,
    DNP_E_ST_PEER_STATE_OUT_COMPLETE

} DNP_E_PEER_STATE, *PDNP_E_PEER_STATE;

#define DNP_ST_STATE_TIMEOUT(tmCurTime, nSeconds) (tmCurTime - tmStateBeginTime >= nSeconds || tmCurTime < tmStateBeginTime)

class CStressTestNetPeer
{
    friend class CStressTestMsgWorkThread;
public:
    CStressTestNetPeer(CStressTestMsgWorkThread *pMsgWorkThreadIn, CStressTestBbAddrPool *pBbAddrPoolIn, uint32 nMsgMagicIn, 
        bool fInBoundIn, uint64 nNetIdIn, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn);
    ~CStressTestNetPeer();

    bool StressTestDoRecvPacket(CMthNetPackData *pPackData);
    bool StressTestDoOutBoundConnectSuccess(CMthNetEndpoint& tLocalEp);

private:
    bool StressTestDoPeerPacket();

    void StressTestModifyPeerState(DNP_E_PEER_STATE eState);
    bool StressTestSendMessage(int nChannel, int nCommand, CBlockheadBufStream& ssPayload);
    bool StressTestDoStateTimer(time_t tmCurTime);

    bool StressTestSendMsgHello();
    bool StressTestSendMsgHelloAck();

    bool StressTestSendMsgGetAddress();
    bool StressTestSendMsgAddress();

private:
    bool fInBound;
    DNP_E_PEER_STATE ePeerState;
    time_t tmStateBeginTime;

    uint32 nMsgMagic;
    CStressTestMsgWorkThread *pMsgWorkThread;
    CStressTestBbAddrPool *pBbAddrPool;

    uint64 nPeerNetId;
    CMthNetEndpoint tPeerEp;
    CMthNetEndpoint tLocalEp;

    CProtoDataBuf tRecvDataBuf;

    int nVersion;
    uint64 nService;
    uint64 nNonceFrom;
    int64 nTimeDelta;
    int64 nSendHelloTime;
    std::string strSubVer;
    int nStartingHeight;

    bool fGetPeerAddress;
    bool fIfNeedRespGetAddress;
};

#define HSS_MAX_STRESS_LISTEN_PORT_COUNT 50000

class CStressTestBbAddrPool
{
public:
    CStressTestBbAddrPool(CStressTestConfig *pCfg, CNetWorkService *pNetWS);
    ~CStressTestBbAddrPool();

    bool StressTestStartAllTcpListen();
    bool StressTestAddRecvAddr(tcp::endpoint& ep, uint64 nServiceIn);
    bool StressTestUpdateHeight(CMthNetEndpoint &ep, int iHeight);

    void StressTestGetGoodAddressList(vector<CAddress>& vAddrList);
    bool StressTestGetCallConnectAddrList(vector<CBbAddr>& vBbAddr, uint32 nGetAddrCount);

    int64 StressTestGetNetTime();
    bool StressTestUpdateNetTime(const boost::asio::ip::address& address,int64 nTimeDelta);

    void StressTestSetConfidentHeight(uint32 nHeight);
    uint32 StressTestGetConfidentHeight();

private:
    CStressTestConfig *pTestCfg;
    CNetWorkService *pNetWorkService;

    map<string, CBbAddr*> mapAddrPool;
    boost::shared_mutex lockAddrPool;

    uint16 nListenPortTable[HSS_MAX_STRESS_LISTEN_PORT_COUNT];
    uint32 nCurGetPortPos;
    uint32 nCurConnectCount;
};


class CStressTestService
{
public:
    CStressTestService();
    ~CStressTestService();

    bool StartService(int argc, char *argv[]);
    void StopService();

private:
    boost::filesystem::path GetDefaultDataDir();

    bool Start();
    void Stop();
    void Work();
    void Stat();

public:
    CStressTestConfig tStressTestCfg;
    CNetWorkService *pNetWorkService;
    CStressTestWorkThreadPool *pWorkThreadPool;
    CStressTestBbAddrPool *pBbAddrPool;

    boost::thread *pThreadWork;
    bool fRunFlag;

    CStressTestStat tPrevStatData;
    time_t tmPrevStatTime;
};


///////////////////////////////////////////////////////////////////////////
int main_stress_test(int argc,char **argv);

} //namespace stresstest

#endif
