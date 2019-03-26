// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __NETWORK_NETWORKTHREAD_H
#define __NETWORK_NETWORKTHREAD_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include "blockhead/type.h"
#include "blockhead/util.h"
#include "nbase/mthbase.h"
#include "networkbase.h"


namespace network
{

using namespace nbase;
using boost::asio::ip::tcp;

class CNetWorkService;
class CTcpConnect;
class CMthNetPackData;


class CNetWorkThread
{
    friend class CTcpConnect;
public:
    CNetWorkThread(uint16 usThreadWorkIdIn, CNetWorkService& inNetWork);
    ~CNetWorkThread();

    uint16 GetThreadId() const {return usThreadWorkId;}
    boost::asio::io_service& GetIoService() {return ioServiceClientWork;}

    bool Start();
    void Stop();

    CNetDataQueue& GetRecvQueue() {return qRecvQueue;}

    void Disconnect(CTcpConnect* pTcpConnIn);
    void RemoveTcpConnect(CTcpConnect* pTcpConnIn);
    bool PostTcpConnectRequest(CMthNetEndpoint& epPeer, uint64& nNetIdOut);
    void PostAccept(CTcpConnect* pTcpConnect);
    void PostSendData(CMthNetPackData* pNvBuf);
    void PostRecvRequest(uint64 nNetId);

private:
    void Work();

    void DoTcpRemoveTimer();

    void HandleTimer(const boost::system::error_code &err);
    void HandleAccept(CTcpConnect* pTcpConnect);
    void HandleSendData(CMthNetPackData* pNvBuf);
    void HandleRecvRequest(uint64 nNetId);
    void HandleConnectCompleted(CTcpConnect* pTcpConnect, const boost::system::error_code &ec);

private:
    CNetWorkService& tNetWorkService;
    uint16 usThreadWorkId;

    boost::thread *pThreadClientWork;
    boost::asio::io_service ioServiceClientWork;
    boost::asio::io_service::work workClientWork;
    boost::asio::deadline_timer *pTimerClientWork;

    std::map<uint64,CTcpConnect*> mapTcpConn;
    std::map<uint64,CTcpConnect*> mapTcpRemove;

    CNetDataQueue qRecvQueue;
};



}  // namespace network

#endif //__NETWORK_NETWORKTHREAD_H
