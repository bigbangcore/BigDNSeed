// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netpeer.h"

#include "netmsgwork.h"
#include "version.h"

using boost::asio::ip::tcp;

namespace dnseed
{

//-----------------------------------------------------------------------------------------------
CNetPeer::CNetPeer(CMsgWorkThread* pMsgWorkThreadIn, CBbAddrPool* pBbAddrPoolIn, uint32 nMsgMagicIn,
                   bool fInBoundIn, uint64 nNetIdIn, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, bool fAllowAllAddrIn)
  : pMsgWorkThread(pMsgWorkThreadIn), pBbAddrPool(pBbAddrPoolIn), nMsgMagic(nMsgMagicIn),
    fInBound(fInBoundIn), nPeerNetId(nNetIdIn), tPeerEp(tPeerEpIn), tLocalEp(tLocalEpIn), fPeerAllowAllAddr(fAllowAllAddrIn)
{
    nVersion = 0;
    nService = 0;
    nNonceFrom = 0;
    nTimeDelta = 0;
    nSendHelloTime = 0;
    nStartingHeight = 0;
    fGetPeerAddress = false;
    fIfNeedRespGetAddress = false;
    fIfDoComplete = false;

    if (fInBoundIn)
    {
        ModifyPeerState(DNP_E_PEER_STATE_IN_CONNECTED);
    }
    else
    {
        ModifyPeerState(DNP_E_PEER_STATE_OUT_CONNECTING);
    }
}

CNetPeer::~CNetPeer()
{
}

bool CNetPeer::DoRecvPacket(CMthNetPackData* pPackData)
{
    if (pPackData == NULL || pPackData->GetDataBuf() == NULL || pPackData->GetDataLen() == 0)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Param error.");
        return false;
    }
    tRecvDataBuf += *pPackData;

    while (tRecvDataBuf.CheckPacketIntegrity())
    {
        if (!tRecvDataBuf.VerifyPacket(nMsgMagic))
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "VerifyPacket fail.");
            return false;
        }
        if (tRecvDataBuf.GetChannel() == BBPROTO_CHN_NETWORK)
        {
            if (!DoPacket())
            {
                return false;
            }
        }
        tRecvDataBuf.ErasePacket();
    }
    return true;
}

bool CNetPeer::DoOutBoundConnectSuccess(CMthNetEndpoint& tLocalEpIn)
{
    tLocalEp = tLocalEpIn;
    ModifyPeerState(DNP_E_PEER_STATE_OUT_CONNECTED);
    if (!SendMsgHello())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Send hello message fail.");
        return false;
    }
    ModifyPeerState(DNP_E_PEER_STATE_OUT_WAIT_HELLO);
    return true;
}

//------------------------------------------------------------------------------------
bool CNetPeer::DoPacket()
{
    int64 nTimeRecv = GetTime();
    switch (tRecvDataBuf.GetCommand())
    {
    case BBPROTO_CMD_HELLO:
    {
        try
        {
            blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_HELLO");
            if (nVersion != 0)
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "nVersion error.");
                return false;
            }

            CBlockheadBufStream ssPayload;
            if (!tRecvDataBuf.GetPayload(ssPayload))
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "GetPayload fail.");
                return false;
            }

            int64 nTime;
            uint256 hashRecvGenesisBlock;
            ssPayload >> nVersion >> nService >> nTime >> nNonceFrom >> strSubVer >> nStartingHeight >> hashRecvGenesisBlock;
            nTimeDelta = nTime - nTimeRecv;

            if (STD_DEBUG)
            {
                string sInfo = string("hello peer: ") + tPeerEp.ToString() + string(", height: ") + to_string(nStartingHeight);
                blockhead::StdDebug("CFLOW", sInfo.c_str());
            }

            if (fInBound)
            {
                if (ePeerState == DNP_E_PEER_STATE_IN_CONNECTED)
                {
                    if (!SendMsgHello())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgHello fail.");
                        return false;
                    }
                    ModifyPeerState(DNP_E_PEER_STATE_IN_WAIT_HELLO_ACK);
                }
                else
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO in state error.");
                    return false;
                }
            }
            else
            {
                if (ePeerState == DNP_E_PEER_STATE_OUT_WAIT_HELLO)
                {
                    ModifyPeerState(DNP_E_PEER_STATE_OUT_HANDSHAKED_COMPLETE);
                    nTimeDelta += (nTimeRecv - nSendHelloTime) / 2;
                    pMsgWorkThread->HandlePeerHandshaked(this);

                    if (!SendMsgHelloAck())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgHelloAck fail.");
                        return false;
                    }
                    if (!SendMsgGetAddress())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgGetAddress fail.");
                        return false;
                    }
                    ModifyPeerState(DNP_E_PEER_STATE_OUT_WAIT_ADDRESS_RSP);
                }
                else
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO out state error.");
                    return false;
                }
            }
        }
        catch (exception& e)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        break;
    }
    case BBPROTO_CMD_HELLO_ACK:
    {
        blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_HELLO_ACK");
        if (fInBound)
        {
            if (ePeerState == DNP_E_PEER_STATE_IN_WAIT_HELLO_ACK)
            {
                if (nVersion == 0)
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "nVersion error.");
                    return false;
                }
                ModifyPeerState(DNP_E_PEER_STATE_IN_HANDSHAKED_COMPLETE);
                nTimeDelta += (nTimeRecv - nSendHelloTime) / 2;
                pMsgWorkThread->HandlePeerHandshaked(this);
                if (!SendMsgGetAddress())
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgGetAddress fail.");
                    return false;
                }
                ModifyPeerState(DNP_E_PEER_STATE_IN_WAIT_ADDRESS_RSP);
            }
            else
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO_ACK in state error.");
                return false;
            }
        }
        break;
    }
    case BBPROTO_CMD_GETADDRESS:
    {
        blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_GETADDRESS");
        if (!SendMsgAddress())
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgAddress fail.");
            return false;
        }
        /*if(!fGetPeerAddress)
        {
            fIfNeedRespGetAddress = true;
        }
        else
        {
            if (!SendMsgAddress())
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgAddress fail.");
                return false;
            }
        }*/
        break;
    }
    case BBPROTO_CMD_ADDRESS:
    {
        vector<CAddress> vAddrList;
        CBlockheadBufStream ssPayload;
        if (!tRecvDataBuf.GetPayload(ssPayload))
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "GetPayload fail.");
            return false;
        }
        ssPayload >> vAddrList;

        if (STD_DEBUG)
        {
            string sInfo = string("Recv address count: ") + to_string(vAddrList.size());
            blockhead::StdDebug("CFLOW", sInfo.c_str());
        }

        for (vector<CAddress>::iterator it = vAddrList.begin(); it != vAddrList.end(); ++it)
        {
            if (((*it).nService & NODE_NETWORK) == NODE_NETWORK && (fPeerAllowAllAddr || (*it).ssEndpoint.IsRoutable()))
            {
                tcp::endpoint ep;
                (*it).ssEndpoint.GetEndpoint(ep);
                pBbAddrPool->AddRecvAddr(ep, (*it).nService);

                /*if (STD_DEBUG)
                {
                    string strAddAddr = string("Address: ") + ep.address().to_string() + ":" + to_string(ep.port());
                    blockhead::StdDebug("CFLOW", strAddAddr.c_str());
                }*/
            }
        }
        /*fGetPeerAddress = true;

        if (fIfNeedRespGetAddress)
        {
            if (!SendMsgAddress())
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "SendMsgAddress fail.");
                return false;
            }
        }*/

        if (ePeerState == DNP_E_PEER_STATE_IN_WAIT_ADDRESS_RSP)
        {
            ModifyPeerState(DNP_E_PEER_STATE_IN_COMPLETE);
            fIfDoComplete = true;
        }
        else if (ePeerState == DNP_E_PEER_STATE_OUT_WAIT_ADDRESS_RSP)
        {
            ModifyPeerState(DNP_E_PEER_STATE_OUT_COMPLETE);
            fIfDoComplete = true;
        }
        else
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_ADDRESS stat error.");
            return false;
        }
        break;
    }
    default:
        blockhead::StdError(__PRETTY_FUNCTION__, "Error message.");
        break;
    }

    return true;
}

void CNetPeer::ModifyPeerState(DNP_E_PEER_STATE eState)
{
    ePeerState = eState;
    tmStateBeginTime = time(NULL);
}

bool CNetPeer::SendMessage(int nChannel, int nCommand, CBlockheadBufStream& ssPayload)
{
    CProtoDataBuf tPacketBuf(nMsgMagic, nChannel, nCommand, ssPayload);
    return pMsgWorkThread->SendDataPacket(nPeerNetId, tPeerEp, tLocalEp, tPacketBuf.GetDataBuf(), tPacketBuf.GetDataLen());
}

bool CNetPeer::DoStateTimer(time_t tmCurTime)
{
    switch (ePeerState)
    {
    case DNP_E_PEER_STATE_INIT:
        break;

    case DNP_E_PEER_STATE_IN_CONNECTED:
        if (DNP_STATE_TIMEOUT(tmCurTime, 30))
        {
            blockhead::StdDebug("CFLOW", "In connect timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_IN_WAIT_HELLO_ACK:
        if (DNP_STATE_TIMEOUT(tmCurTime, 10))
        {
            blockhead::StdDebug("CFLOW", "In connect wait hello ack timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_IN_HANDSHAKED_COMPLETE:
        if (DNP_STATE_TIMEOUT(tmCurTime, 30))
        {
            blockhead::StdDebug("CFLOW", "In connect handshaked complete timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_IN_WAIT_ADDRESS_RSP:
        if (DNP_STATE_TIMEOUT(tmCurTime, 10))
        {
            blockhead::StdDebug("CFLOW", "In connect wait address rsp timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_IN_COMPLETE:
        if (DNP_STATE_TIMEOUT(tmCurTime, 2))
        {
            blockhead::StdDebug("CFLOW", "In connect work complete.");
            return false;
        }
        break;

    case DNP_E_PEER_STATE_OUT_CONNECTING:
        if (DNP_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "Out connecting timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_OUT_CONNECTED:
        if (DNP_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "Out connect connected timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_OUT_WAIT_HELLO:
        if (DNP_STATE_TIMEOUT(tmCurTime, 10))
        {
            blockhead::StdDebug("CFLOW", "Out connect wait hello timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_OUT_HANDSHAKED_COMPLETE:
        if (DNP_STATE_TIMEOUT(tmCurTime, 30))
        {
            blockhead::StdDebug("CFLOW", "Out connect handshaked complete timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_OUT_WAIT_ADDRESS_RSP:
        if (DNP_STATE_TIMEOUT(tmCurTime, 10))
        {
            blockhead::StdDebug("CFLOW", "Out connect wait address rsp timeout.");
            return false;
        }
        break;
    case DNP_E_PEER_STATE_OUT_COMPLETE:
        if (DNP_STATE_TIMEOUT(tmCurTime, 2))
        {
            blockhead::StdDebug("CFLOW", "Out connect work complete.");
            return false;
        }
        break;
    }
    return true;
}

//-----------------------------------------------------------------------------------
bool CNetPeer::SendMsgHello()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_HELLO");
    CBlockheadBufStream ssPayload;
    int iLocalVersion = PROTO_VERSION;
    uint64 nLocalService = NODE_NETWORK;
    string strLocalSubVer = FormatSubVersion();

    uint64 nNonce = nPeerNetId;
    int64 nTime = pBbAddrPool->GetNetTime();
    int nHeight = pBbAddrPool->GetConfidentHeight();
    ssPayload << iLocalVersion << nLocalService << nTime << nNonce << strLocalSubVer << nHeight << pBbAddrPool->GetGenesisBlockHash();

    nSendHelloTime = GetTime();
    return SendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_HELLO, ssPayload);
}

bool CNetPeer::SendMsgHelloAck()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_HELLO_ACK");
    CBlockheadBufStream ssPayload;
    return SendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_HELLO_ACK, ssPayload);
}

bool CNetPeer::SendMsgGetAddress()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_GETADDRESS");
    CBlockheadBufStream ssPayload;
    return SendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_GETADDRESS, ssPayload);
}

bool CNetPeer::SendMsgAddress()
{
    CBlockheadBufStream ssPayload;
    vector<CAddress> vAddrList;
    pBbAddrPool->GetGoodAddressList(vAddrList);

    if (STD_DEBUG)
    {
        string sInfo = string("Send msg: BBPROTO_CMD_ADDRESS, addr count: ") + to_string(vAddrList.size());
        blockhead::StdDebug("CFLOW", sInfo.c_str());
    }

    ssPayload << vAddrList;
    return SendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_ADDRESS, ssPayload);
}

} // namespace dnseed
