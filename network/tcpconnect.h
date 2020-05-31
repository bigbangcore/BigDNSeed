// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __NETWORK_TCPCONNECT_H
#define __NETWORK_TCPCONNECT_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>

#include "blockhead/type.h"
#include "nbase/mthbase.h"
#include "networkbase.h"

namespace network
{

using namespace nbase;
using boost::asio::ip::tcp;

class CNetWorkThread;
class CMthNetPackData;

class CTcpConnect
{
    friend class CNetWorkService;
    friend class CNetWorkThread;

public:
    CTcpConnect(CNetWorkThread& inNetWorkThread, uint64 nListenNetId, CMthNetEndpoint& epListen);
    CTcpConnect(CNetWorkThread& inNetWorkThread, CMthNetEndpoint& epPeer);
    ~CTcpConnect();

    uint64 GetTcpConnNetId() const
    {
        return ui64TcpConnNetId;
    }
    tcp::socket& GetSocketId()
    {
        return socketClient;
    }

    bool Accept();
    void TcpRemove(E_DISCONNECT_CAUSE eCloseCause);
    void SetSendData(CMthNetPackData* pNvBuf);
    void PostRecvRequest();
    void PostSendRequest();
    bool ConnectCompleted();
    void ConnectFail();

    void DoRemoveTimer();

private:
    void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
    void HandleWrite(const boost::system::error_code& ec, std::size_t bytes_transferred);

private:
    bool fInBound;
    bool fIfOpenSocket;
    uint32 nRefCount;
    time_t tmCloseTime;

    uint64 ui64TcpConnNetId;
    uint64 ui64LinkListenNetId;
    CMthNetEndpoint epLinkListen;
    tcp::socket socketClient;
    CMthNetEndpoint epPeer;
    CMthNetEndpoint epLocal;

    CNetWorkThread& tNetWorkThread;

    char buffer_read[1450];

    std::queue<CMthNetPackData*> qWaitSendQueue;
    CMthNetPackData* pCurSendNvBuf;

    uint32 nPostRecvOpCount;
};

} // namespace network

#endif //__NETWORK_TCPCONNECT_H
