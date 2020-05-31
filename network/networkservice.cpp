// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkservice.h"

#include "nbase/mthbase.h"
#include "networkthread.h"
#include "tcpconnect.h"

using namespace std;
using namespace nbase;

namespace network
{

//---------------------------------------------------------------------------------------------------------
CTcpListenNode::CTcpListenNode(boost::asio::io_service& io, CMthNetEndpoint& addr)
  : acceptorService(io), tListenAddr(addr)
{
    ui64ListenNetId = CBaseUniqueId::CreateUniqueId(0, 0, 1, 0);
}

CTcpListenNode::~CTcpListenNode()
{
    StopListen();
}

bool CTcpListenNode::StartListen()
{
    try
    {
        if (tListenAddr.GetIpType() == CMthNetIp::MNI_IPV4)
        {
            tcp::endpoint epListen(boost::asio::ip::address::from_string(tListenAddr.GetIp()), tListenAddr.GetPort());

            acceptorService.open(epListen.protocol());
            acceptorService.set_option(tcp::acceptor::reuse_address(true));

            boost::system::error_code ec;
            acceptorService.bind(epListen, ec);
            if (ec)
            {
                throw runtime_error((string("Tcp bind fail, addr: ") + epListen.address().to_string() + string(":") + to_string(epListen.port()) + string(", cause: ") + ec.message()).c_str());
            }
        }
        else if (tListenAddr.GetIpType() == CMthNetIp::MNI_IPV6)
        {
            tcp::endpoint epListen(boost::asio::ip::address_v6::from_string(tListenAddr.GetIp()), tListenAddr.GetPort());

            acceptorService.open(epListen.protocol());
            acceptorService.set_option(tcp::acceptor::reuse_address(true));
            acceptorService.set_option(boost::asio::ip::v6_only(true));

            boost::system::error_code ec;
            acceptorService.bind(epListen, ec);
            if (ec)
            {
                throw runtime_error((string("Tcp bind fail, addr: ") + epListen.address().to_string() + string(":") + to_string(epListen.port()) + string(", cause: ") + ec.message()).c_str());
            }
        }
        else
        {
            throw runtime_error(string("Ip type error."));
        }

        if (tListenAddr.GetPort() == 0)
        {
            tcp::endpoint epLocal = acceptorService.local_endpoint();
            tListenAddr.SetPort(epLocal.port());
        }

        acceptorService.listen();
    }
    catch (exception& e)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, e.what());
        acceptorService.close();
        return false;
    }

    return true;
}

void CTcpListenNode::StopListen()
{
    acceptorService.close();
}

//---------------------------------------------------------------------------------------------------------
CNetWorkService::CNetWorkService(uint32 nWorkThreadCount)
  : workListen(ioServiceListen), pThreadListenWork(NULL)
{
    if (nWorkThreadCount == 0)
    {
        ui32NetWorkThreadCount = boost::thread::hardware_concurrency();
        if (ui32NetWorkThreadCount == 0)
        {
            ui32NetWorkThreadCount = 1;
        }
    }
    else
    {
        ui32NetWorkThreadCount = nWorkThreadCount;
        if (ui32NetWorkThreadCount > MAX_NET_WORK_THREAD_COUNT)
        {
            ui32NetWorkThreadCount = MAX_NET_WORK_THREAD_COUNT;
        }
    }

    for (int i = 0; i < ui32NetWorkThreadCount; i++)
    {
        pNetworkThreadTable[i] = new CNetWorkThread(i, *this);
        ui32PortStatTable[i] = 0;
    }
}

CNetWorkService::~CNetWorkService()
{
    for (int i = 0; i < ui32NetWorkThreadCount; i++)
    {
        delete pNetworkThreadTable[i];
        pNetworkThreadTable[i] = NULL;
    }
}

//------------------------------------------------------------------------------------
bool CNetWorkService::StartService()
{
    if (!StartAllNetWorkThread())
    {
        return false;
    }

    pThreadListenWork = new boost::thread(boost::bind(&CNetWorkService::ListenWork, this));
    if (pThreadListenWork == NULL)
    {
        StopAllNetWorkThread();
        return false;
    }

    return true;
}

void CNetWorkService::StopService()
{
    ioServiceListen.stop();

    if (pThreadListenWork)
    {
        pThreadListenWork->join();

        delete pThreadListenWork;
        pThreadListenWork = NULL;
    }

    StopAllNetWorkThread();
    RemoveAllTcpListen();
}

uint64 CNetWorkService::AddTcpIPV4Listen(uint16 nListenPort)
{
    CMthNetEndpoint aListenAddr;
    aListenAddr.SetAddrPort("0.0.0.0", nListenPort);
    return AddTcpListen(aListenAddr);
}

uint64 CNetWorkService::AddTcpIPV6Listen(uint16 nListenPort)
{
    CMthNetEndpoint aListenAddr;
    aListenAddr.SetAddrPort("::", nListenPort);
    return AddTcpListen(aListenAddr);
}

uint64 CNetWorkService::AddTcpListen(CMthNetEndpoint& addrListen)
{
    CTcpListenNode* pNode;
    uint64 nNetId;

    nNetId = QueryTcpListenNetId(addrListen);
    if (nNetId)
    {
        return nNetId;
    }

    pNode = new CTcpListenNode(ioServiceListen, addrListen);
    if (pNode == NULL)
    {
        return 0;
    }

    if (!mapTcpListen.insert(make_pair(pNode->GetNetId(), pNode)).second)
    {
        delete pNode;
        return 0;
    }

    if (!pNode->StartListen())
    {
        mapTcpListen.erase(pNode->GetNetId());
        delete pNode;
        return 0;
    }

    for (int i = 0; i < ui32NetWorkThreadCount; i++)
    {
        PostMinAccept(pNode->GetNetId(), addrListen);
    }

    return pNode->GetNetId();
}

void CNetWorkService::RemoveTcpListen(uint64 nTcpListenNetId)
{
    std::map<uint64, CTcpListenNode*>::iterator it;
    it = mapTcpListen.find(nTcpListenNetId);
    if (it != mapTcpListen.end())
    {
        CTcpListenNode* pNode = (*it).second;
        mapTcpListen.erase(nTcpListenNetId);
        if (pNode)
        {
            pNode->StopListen();
            delete pNode;
            pNode = NULL;
        }
    }
}

uint64 CNetWorkService::QueryTcpListenNetId(CMthNetEndpoint& addrListen)
{
    CTcpListenNode* pNode;
    std::map<uint64, CTcpListenNode*>::iterator it;
    for (it = mapTcpListen.begin(); it != mapTcpListen.end(); it++)
    {
        pNode = (*it).second;
        if (pNode && pNode->GetListenAddr() == addrListen)
        {
            return pNode->GetNetId();
        }
    }
    return 0;
}

bool CNetWorkService::QueryTcpListenAddress(uint64 nListenNetId, CMthNetEndpoint& addrListenOut)
{
    std::map<uint64, CTcpListenNode*>::iterator it;
    it = mapTcpListen.find(nListenNetId);
    if (it != mapTcpListen.end())
    {
        CTcpListenNode* pNode;
        pNode = it->second;
        if (pNode)
        {
            addrListenOut = pNode->GetListenAddr();
            return true;
        }
    }
    return false;
}

CNetDataQueue& CNetWorkService::GetWorkRecvQueue(uint32 nWorkThreadIndex)
{
    if (nWorkThreadIndex >= ui32NetWorkThreadCount)
    {
        nWorkThreadIndex = ui32NetWorkThreadCount - 1;
    }
    return pNetworkThreadTable[nWorkThreadIndex]->GetRecvQueue();
}

bool CNetWorkService::ReqSendData(CMthNetPackData* pNvBuf)
{
    if (pNvBuf == NULL)
    {
        return false;
    }
    CNetWorkThread* pNwThread = GetNetWorkThreadByNetId(pNvBuf->ui64NetId);
    if (pNwThread == NULL)
    {
        return false;
    }
    pNwThread->PostSendData(pNvBuf);
    return true;
}

bool CNetWorkService::ReqTcpConnect(CMthNetEndpoint& epPeer, uint64& nNetIdOut)
{
    uint32 nWorkThreadIndex = AddStatNetPort();
    if (pNetworkThreadTable[nWorkThreadIndex] == NULL)
    {
        string sErrorInfo = string("Error net work thread table, index: ") + to_string(nWorkThreadIndex);
        blockhead::StdError(__PRETTY_FUNCTION__, sErrorInfo.c_str());
        RemoveStatNetPort(nWorkThreadIndex);
        return false;
    }
    if (!pNetworkThreadTable[nWorkThreadIndex]->PostTcpConnectRequest(epPeer, nNetIdOut))
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "PostTcpConnectRequest fail.");
        RemoveStatNetPort(nWorkThreadIndex);
        return false;
    }
    return true;
}

void CNetWorkService::RemoveNetPort(uint64 nNetId)
{
    CNetWorkThread* pNwThread = GetNetWorkThreadByNetId(nNetId);
    if (pNwThread)
    {
        CMthNetEndpoint tPeer;
        CMthNetEndpoint tLocal;
        CMthNetPackData* pNvBuf = new CMthNetPackData(nNetId, NET_MSG_TYPE_CLOSE, NET_DIS_CAUSE_LOCAL_CLOSE, tPeer, tLocal);
        pNwThread->PostSendData(pNvBuf);
    }
}

//--------------------------------------------------------------------------------------------
void CNetWorkService::ListenWork()
{
    ioServiceListen.run();
}

bool CNetWorkService::StartAllNetWorkThread()
{
    for (uint32 i = 0; i < ui32NetWorkThreadCount; i++)
    {
        if (!pNetworkThreadTable[i]->Start())
        {
            StopAllNetWorkThread();
            return false;
        }
    }
    return true;
}

void CNetWorkService::StopAllNetWorkThread()
{
    for (uint32 i = 0; i < ui32NetWorkThreadCount; i++)
    {
        pNetworkThreadTable[i]->Stop();
    }
}

void CNetWorkService::RemoveAllTcpListen()
{
    std::map<uint64, CTcpListenNode*>::iterator it;
    for (it = mapTcpListen.begin(); it != mapTcpListen.end();)
    {
        uint64 nNetId = it->first;
        it++;
        RemoveTcpListen(nNetId);
    }
}

CNetWorkThread* CNetWorkService::GetNetWorkThreadByNetId(uint64 ui64NetId)
{
    CBaseUniqueId tNetId(ui64NetId);
    uint32 nIndex = tNetId.GetType();
    if (nIndex >= ui32NetWorkThreadCount)
    {
        nIndex = ui32NetWorkThreadCount - 1;
    }
    return pNetworkThreadTable[nIndex];
}

uint32 CNetWorkService::AddStatNetPort()
{
    boost::unique_lock<boost::shared_mutex> writeLock(lockStatCount);
    uint32 uiMinCount = 0xFFFFFFFF;
    uint32 uiMinIndex = 0;
    for (uint32 i = 0; i < ui32NetWorkThreadCount; i++)
    {
        if (ui32PortStatTable[i] < uiMinCount)
        {
            uiMinCount = ui32PortStatTable[i];
            uiMinIndex = i;
        }
    }
    ui32PortStatTable[uiMinIndex]++;
    return uiMinIndex;
}

void CNetWorkService::RemoveStatNetPort(uint32 nWorkThreadIndex)
{
    boost::unique_lock<boost::shared_mutex> writeLock(lockStatCount);
    if (nWorkThreadIndex >= ui32NetWorkThreadCount)
    {
        nWorkThreadIndex = ui32NetWorkThreadCount - 1;
    }
    ui32PortStatTable[nWorkThreadIndex]--;
}

void CNetWorkService::PostMinAccept(uint64 nListenNetId, CMthNetEndpoint& epListen)
{
    uint32 nWorkThreadIndex = AddStatNetPort();
    if (pNetworkThreadTable[nWorkThreadIndex])
    {
        CTcpConnect* pTcpConnect = new CTcpConnect(*(pNetworkThreadTable[nWorkThreadIndex]), nListenNetId, epListen);
        if (pTcpConnect)
        {
            std::map<uint64, CTcpListenNode*>::iterator it = mapTcpListen.find(nListenNetId);
            if (it != mapTcpListen.end() && it->second)
            {
                it->second->GetAcceptor().async_accept(pTcpConnect->socketClient,
                                                       boost::bind(&CNetWorkService::HandleListenAccept, this,
                                                                   pTcpConnect, boost::asio::placeholders::error));
                return;
            }
            delete pTcpConnect;
        }
    }
    RemoveStatNetPort(nWorkThreadIndex);
}

void CNetWorkService::HandleListenAccept(CTcpConnect* pTcpConnect, const boost::system::error_code& ec)
{
    if (pTcpConnect)
    {
        PostMinAccept(pTcpConnect->ui64LinkListenNetId, pTcpConnect->epLinkListen);
        if (!ec)
        {
            CNetWorkThread* pNetWorkThread = GetNetWorkThreadByNetId(pTcpConnect->ui64TcpConnNetId);
            if (pNetWorkThread)
            {
                pNetWorkThread->PostAccept(pTcpConnect);
                return;
            }
        }
        CBaseUniqueId tNetId(pTcpConnect->ui64TcpConnNetId);
        uint32 nThreadIndex = tNetId.GetType();
        RemoveStatNetPort(nThreadIndex);
        delete pTcpConnect;
    }
    else
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "pTcpConnect is NULL.");
    }
}

} // namespace network
