// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_NETMSGWORK_H
#define __DNSEED_NETMSGWORK_H

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <iostream>

#include "addrpool.h"
#include "blockhead/type.h"
#include "config.h"
#include "netpeer.h"
#include "netproto.h"
#include "network/networkservice.h"

namespace dnseed
{

using namespace std;
using namespace network;
using namespace blockhead;

class CMsgWorkThread;

class CWorkThreadPool
{
public:
    CWorkThreadPool(CDnseedConfig* pCfg, CNetWorkService* nws, CBbAddrPool* pBbAddrPoolIn);
    ~CWorkThreadPool();

    bool StartAll();
    void StopAll();

    bool StatRunData(CRunStatData& tStatData);

private:
    CMsgWorkThread* pWorkThreadTable[MAX_NET_WORK_THREAD_COUNT];
    uint32 nWorkThreadCount;

    CDnseedConfig* pDNSeedCfg;
    CNetWorkService* pNetWorkService;
    CBbAddrPool* pBbAddrPool;
};

class CMsgWorkThread
{
    friend class CNetPeer;

public:
    CMsgWorkThread(uint32 nThreadCount, uint32 nThreadIndex, CDnseedConfig* pCfg, CNetWorkService* nws, CBbAddrPool* pBbAddrPoolIn);
    ~CMsgWorkThread();

    bool Start();
    void Stop();

    void GetNetStatData(CRunStatData& tStatOut);

private:
    void Work();
    void Timer();

    void DoPacket(CMthNetPackData* pPackData);
    bool SendDataPacket(uint64 nNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, char* pBuf, uint32 nLen);

    CNetPeer* AddNetPeer(bool fInBoundIn, uint64 nPeerNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn);
    void DelNetPeer(uint64 nPeerNetId);
    CNetPeer* GetNetPeer(uint64 nPeerNetId);

    void ActiveClosePeer(uint64 nPeerNetId);
    bool HandlePeerHandshaked(CNetPeer* pPeer);

    void DoCallConnect();
    void DoPeerState(time_t tmCurTime);

    void StartConnectPeer(CBbAddr& tBbAddr);

private:
    bool fRunFlag;
    boost::thread* pThreadMsgWork;

    uint32 nWorkThreadCount;
    uint32 nWorkThreadIndex;
    CDnseedConfig* pDNSeedCfg;
    CNetWorkService* pNetWorkService;
    CBbAddrPool* pBbAddrPool;
    CNetDataQueue* pNetDataQueue;
    uint32 nPersCalloutAddrCount;

    int64 nStartBackTestTime;
    uint64 nCompBackTestCount;

    map<uint64, CNetPeer*> mapPeer;

    CRunStatData tNetStatData;
    boost::shared_mutex lockStat;

    time_t tmPrevTimerDoTime;
};

} //namespace dnseed

#endif //__DNSEED_NETMSGWORK_H
