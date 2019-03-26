// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_NETPEER_H
#define __DNSEED_NETPEER_H

#include <iostream>
#include "blockhead/type.h"
#include "network/networkbase.h"
#include "netproto.h"
#include "addrpool.h"

namespace dnseed
{

using namespace std;
using namespace blockhead;
using namespace network;

class CMsgWorkThread;

typedef enum _DNP_E_PEER_STATE
{
    DNP_E_PEER_STATE_INIT,

    DNP_E_PEER_STATE_IN_CONNECTED,
    DNP_E_PEER_STATE_IN_WAIT_HELLO_ACK,
    DNP_E_PEER_STATE_IN_HANDSHAKED_COMPLETE,
    DNP_E_PEER_STATE_IN_WAIT_ADDRESS_RSP,
    DNP_E_PEER_STATE_IN_COMPLETE,

    DNP_E_PEER_STATE_OUT_CONNECTING,
    DNP_E_PEER_STATE_OUT_CONNECTED,
    DNP_E_PEER_STATE_OUT_WAIT_HELLO,
    DNP_E_PEER_STATE_OUT_HANDSHAKED_COMPLETE,
    DNP_E_PEER_STATE_OUT_WAIT_ADDRESS_RSP,
    DNP_E_PEER_STATE_OUT_COMPLETE

} DNP_E_PEER_STATE, *PDNP_E_PEER_STATE;

#define DNP_STATE_TIMEOUT(tmCurTime, nSeconds) (tmCurTime - tmStateBeginTime >= nSeconds || tmCurTime < tmStateBeginTime)

class CNetPeer
{
    friend class CMsgWorkThread;
public:
    CNetPeer(CMsgWorkThread *pMsgWorkThreadIn, CBbAddrPool *pBbAddrPoolIn, uint32 nMsgMagicIn, 
        bool fInBoundIn, uint64 nNetIdIn, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, bool fAllowAllAddrIn);
    ~CNetPeer();

    bool DoRecvPacket(CMthNetPackData *pPackData);
    bool DoOutBoundConnectSuccess(CMthNetEndpoint& tLocalEpIn);

private:
    bool DoPacket();

    void ModifyPeerState(DNP_E_PEER_STATE eState);
    bool SendMessage(int nChannel, int nCommand, CBlockheadBufStream& ssPayload);
    bool DoStateTimer(time_t tmCurTime);

    bool SendMsgHello();
    bool SendMsgHelloAck();

    bool SendMsgGetAddress();
    bool SendMsgAddress();

private:
    bool fInBound;
    DNP_E_PEER_STATE ePeerState;
    time_t tmStateBeginTime;

    uint32 nMsgMagic;
    CMsgWorkThread *pMsgWorkThread;
    CBbAddrPool *pBbAddrPool;

    uint64 nPeerNetId;
    CMthNetEndpoint tPeerEp;
    CMthNetEndpoint tLocalEp;
    bool fPeerAllowAllAddr;

    CProtoDataBuf tRecvDataBuf;

    int nVersion;
    uint64 nService;
    uint64 nNonceFrom;
    int64 nTimeDelta;
    int64 nSendHelloTime;
    string strSubVer;
    int nStartingHeight;

    bool fGetPeerAddress;
    bool fIfNeedRespGetAddress;
    bool fIfDoComplete;
};

} //namespace dnseed

#endif //__DNSEED_NETPEER_H
