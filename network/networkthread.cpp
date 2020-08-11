// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkthread.h"

#include "nbase/mthbase.h"
#include "networkservice.h"
#include "tcpconnect.h"

using namespace std;
using namespace nbase;
using namespace blockhead;
using boost::asio::ip::tcp;

namespace network
{

//--------------------------------------------------------------------------------------------------------
CNetWorkThread::CNetWorkThread(uint16 usThreadWorkIdIn, CNetWorkService& inNetWork)
  : usThreadWorkId(usThreadWorkIdIn), tNetWorkService(inNetWork), workClientWork(ioServiceClientWork),
    pThreadClientWork(NULL), pTimerClientWork(NULL)
{
    qRecvQueue.SetLowTrigger(20, boost::bind(&CNetWorkThread::PostRecvRequest, this, _1));
}

CNetWorkThread::~CNetWorkThread()
{
    Stop();
}

bool CNetWorkThread::Start()
{
    pThreadClientWork = new boost::thread(boost::bind(&CNetWorkThread::Work, this));
    if (pThreadClientWork == NULL)
    {
        StdError("NetWorkThread", "Start thread fail.");
        return false;
    }
    return true;
}

void CNetWorkThread::Stop()
{
    ioServiceClientWork.stop();

    if (pThreadClientWork)
    {
        pThreadClientWork->join();

        delete pThreadClientWork;
        pThreadClientWork = NULL;
    }

    if (pTimerClientWork)
    {
        pTimerClientWork->cancel();

        delete pTimerClientWork;
        pTimerClientWork = NULL;
    }
}

//-----------------------------------------------------------------------------------
void CNetWorkThread::Work()
{
    try
    {
        pTimerClientWork = new boost::asio::deadline_timer(ioServiceClientWork, boost::posix_time::seconds(1));
        pTimerClientWork->async_wait(boost::bind(&CNetWorkThread::HandleTimer, this, _1));

        ioServiceClientWork.run();
    }
    catch (boost::thread_interrupted& er)
    {
        StdError("NetWorkThread", "Work error.");
    }
}

void CNetWorkThread::Disconnect(CTcpConnect* pTcpConnIn)
{
    if (pTcpConnIn)
    {
        map<uint64, CTcpConnect*>::iterator it = mapTcpConn.find(pTcpConnIn->GetTcpConnNetId());
        if (it != mapTcpConn.end() && it->second)
        {
            mapTcpRemove.insert(make_pair(pTcpConnIn->GetTcpConnNetId(), pTcpConnIn));
        }
        mapTcpConn.erase(pTcpConnIn->GetTcpConnNetId());
        tNetWorkService.RemoveStatNetPort(usThreadWorkId);
    }
}

void CNetWorkThread::RemoveTcpConnect(CTcpConnect* pTcpConnIn)
{
    if (pTcpConnIn)
    {
        mapTcpRemove.erase(pTcpConnIn->GetTcpConnNetId());
    }
}

bool CNetWorkThread::PostTcpConnectRequest(CMthNetEndpoint& epPeer, uint64& nNetIdOut)
{
    CTcpConnect* pTcpConnect = new CTcpConnect(*this, epPeer);
    if (pTcpConnect == NULL)
    {
        StdError("NetWorkThread", "PostTcpConnectRequest: new CTcpConnect fail.");
        return false;
    }
    pTcpConnect->nRefCount++;
    nNetIdOut = pTcpConnect->GetTcpConnNetId();

    tcp::endpoint epRemote;
    if (!epPeer.GetEndpoint(epRemote))
    {
        StdError("NetWorkThread", "PostTcpConnectRequest: GetEndpoint fail.");
        delete pTcpConnect;
        return false;
    }
    pTcpConnect->GetSocketId().async_connect(epRemote, boost::bind(&CNetWorkThread::HandleConnectCompleted, this,
                                                                   pTcpConnect, boost::asio::placeholders::error));
    return true;
}

void CNetWorkThread::PostAccept(CTcpConnect* pTcpConnect)
{
    ioServiceClientWork.post(boost::bind(&CNetWorkThread::HandleAccept, this, pTcpConnect));
}

void CNetWorkThread::PostSendData(CMthNetPackData* pNvBuf)
{
    ioServiceClientWork.post(boost::bind(&CNetWorkThread::HandleSendData, this, pNvBuf));
}

void CNetWorkThread::PostRecvRequest(uint64 nNetId)
{
    ioServiceClientWork.post(boost::bind(&CNetWorkThread::HandleRecvRequest, this, nNetId));
}

//--------------------------------------------------------------------------------------------
void CNetWorkThread::DoTcpRemoveTimer()
{
    CTcpConnect* pTcpConn;
    map<uint64, CTcpConnect*>::iterator it;
    for (it = mapTcpRemove.begin(); it != mapTcpRemove.end();)
    {
        pTcpConn = it->second;
        it++;
        if (pTcpConn)
        {
            pTcpConn->DoRemoveTimer();
        }
    }
}

//--------------------------------------------------------------------------------------------
void CNetWorkThread::HandleTimer(const boost::system::error_code& err)
{
    if (!err)
    {
        //printf("thread: %d, conn: %d.\n", usThreadWorkId, (int)(mapTcpConn.size()));

        DoTcpRemoveTimer();

        boost::posix_time::seconds needTime(4);
        pTimerClientWork->expires_at(pTimerClientWork->expires_at() + needTime);
        pTimerClientWork->async_wait(boost::bind(&CNetWorkThread::HandleTimer, this, _1));
    }
}

void CNetWorkThread::HandleAccept(CTcpConnect* pTcpConnect)
{
    bool fIfSuccess = false;
    std::map<uint64, CTcpConnect*>::iterator it = mapTcpConn.insert(make_pair(pTcpConnect->GetTcpConnNetId(), pTcpConnect)).first;
    if (it != mapTcpConn.end())
    {
        if (pTcpConnect->Accept())
        {
            fIfSuccess = true;
        }
        else
        {
            mapTcpConn.erase(it);
        }
    }
    if (!fIfSuccess)
    {
        tNetWorkService.RemoveStatNetPort(usThreadWorkId);
        delete pTcpConnect;
    }
}

void CNetWorkThread::HandleSendData(CMthNetPackData* pNvBuf)
{
    if (pNvBuf == NULL)
    {
        StdError("NetWorkThread", "HandleSendData: pNvBuf == NULL.");
        return;
    }
    map<uint64, CTcpConnect*>::iterator it = mapTcpConn.find(pNvBuf->ui64NetId);
    if (it != mapTcpConn.end() && it->second)
    {
        switch (pNvBuf->eMsgType)
        {
        case NET_MSG_TYPE_DATA:
            it->second->SetSendData(pNvBuf);
            return;
        case NET_MSG_TYPE_CLOSE:
            it->second->TcpRemove(NET_DIS_CAUSE_LOCAL_CLOSE);
            break;
        }
    }
    delete pNvBuf;
}

void CNetWorkThread::HandleRecvRequest(uint64 nNetId)
{
    map<uint64, CTcpConnect*>::iterator it = mapTcpConn.find(nNetId);
    if (it != mapTcpConn.end() && it->second)
    {
        it->second->PostRecvRequest();
    }
}

void CNetWorkThread::HandleConnectCompleted(CTcpConnect* pTcpConnect, const boost::system::error_code& ec)
{
    if (pTcpConnect == NULL)
    {
        StdError("NetWorkThread", "HandleConnectCompleted: pTcpConnect error");
        return;
    }

    bool fIfSuccess = false;
    if (!ec)
    {
        std::map<uint64, CTcpConnect*>::iterator it = mapTcpConn.insert(make_pair(pTcpConnect->GetTcpConnNetId(), pTcpConnect)).first;
        if (it != mapTcpConn.end())
        {
            if (pTcpConnect->ConnectCompleted())
            {
                fIfSuccess = true;
            }
            else
            {
                mapTcpConn.erase(it);
            }
        }
    }
    if (!fIfSuccess)
    {
        if (blockhead::STD_DEBUG)
        {
            if (ec)
            {
                string sInfo = string("Connect fail, Peer: ") + pTcpConnect->epPeer.ToString() + string(", Cause: ") + ec.message();
                StdDebug("CFLOW", sInfo.c_str());
            }
            else
            {
                string sInfo = string("Connect fail, Peer: ") + pTcpConnect->epPeer.ToString();
                StdDebug("CFLOW", sInfo.c_str());
            }
        }

        pTcpConnect->ConnectFail();
        tNetWorkService.RemoveStatNetPort(usThreadWorkId);
        delete pTcpConnect;
    }
}

} // namespace network
