// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __NETWORK_NETWORKSERVICE_H
#define __NETWORK_NETWORKSERVICE_H

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

using namespace std;
using namespace nbase;
using boost::asio::ip::tcp;

#define MAX_NET_WORK_THREAD_COUNT 256

class CTcpListenNode
{
public:
    CTcpListenNode(boost::asio::io_service& io, CMthNetEndpoint& addr);
    ~CTcpListenNode();

    bool StartListen();
    void StopListen();

    uint64 GetNetId() const
    {
        return ui64ListenNetId;
    }
    CMthNetEndpoint& GetListenAddr()
    {
        return tListenAddr;
    }
    tcp::acceptor& GetAcceptor()
    {
        return acceptorService;
    }

private:
    uint64 ui64ListenNetId;
    CMthNetEndpoint tListenAddr;
    tcp::acceptor acceptorService;
};

class CNetWorkThread;
class CTcpConnect;

class CNetWorkService
{
    friend class CNetWorkThread;

public:
    CNetWorkService(uint32 nWorkThreadCount = 0);
    ~CNetWorkService();

    bool StartService();
    void StopService();

    uint32 GetWorkThreadCount() const
    {
        return ui32NetWorkThreadCount;
    }

    uint64 AddTcpIPV4Listen(uint16 nListenPort);
    uint64 AddTcpIPV6Listen(uint16 nListenPort);
    uint64 AddTcpListen(CMthNetEndpoint& addrListen);
    void RemoveTcpListen(uint64 nTcpListenNetId);
    uint64 QueryTcpListenNetId(CMthNetEndpoint& addrListen);
    bool QueryTcpListenAddress(uint64 nListenNetId, CMthNetEndpoint& addrListenOut);

    CNetDataQueue& GetWorkRecvQueue(uint32 nWorkThreadIndex);
    bool ReqSendData(CMthNetPackData* pNvBuf);
    bool ReqTcpConnect(CMthNetEndpoint& epPeer, uint64& nNetIdOut);
    void RemoveNetPort(uint64 nNetId);

private:
    void ListenWork();

    bool StartAllNetWorkThread();
    void StopAllNetWorkThread();

    void RemoveAllTcpListen();

    CNetWorkThread* GetNetWorkThreadByNetId(uint64 ui64NetId);

    uint32 AddStatNetPort();
    void RemoveStatNetPort(uint32 nWorkThreadIndex);

    void PostMinAccept(uint64 nListenNetId, CMthNetEndpoint& epListen);

    void HandleListenAccept(CTcpConnect* pTcpConnect, const boost::system::error_code& ec);

private:
    boost::thread* pThreadListenWork;
    boost::asio::io_service ioServiceListen;
    boost::asio::io_service::work workListen;

    uint32 ui32NetWorkThreadCount;
    CNetWorkThread* pNetworkThreadTable[MAX_NET_WORK_THREAD_COUNT];

    uint32 ui32PortStatTable[MAX_NET_WORK_THREAD_COUNT];
    boost::shared_mutex lockStatCount;

    std::map<uint64, CTcpListenNode*> mapTcpListen;
};

} // namespace network

#endif // __NETWORK_NETWORKSERVICE_H
