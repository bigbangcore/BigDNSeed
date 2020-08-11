// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tcpconnect.h"

#include "nbase/mthbase.h"
#include "networkbase.h"
#include "networkservice.h"
#include "networkthread.h"

using namespace std;
using namespace nbase;
using namespace blockhead;

namespace network
{

CTcpConnect::CTcpConnect(CNetWorkThread& inNetWorkThread, uint64 nListenNetId, CMthNetEndpoint& epListen)
  : socketClient(inNetWorkThread.GetIoService()), tNetWorkThread(inNetWorkThread),
    ui64LinkListenNetId(nListenNetId), epLinkListen(epListen), pCurSendNvBuf(NULL), nPostRecvOpCount(0), nRefCount(0)
{
    fInBound = true;
    fIfOpenSocket = true;
    ui64TcpConnNetId = CBaseUniqueId::CreateUniqueId(0, 0, inNetWorkThread.GetThreadId(), 0);
}

CTcpConnect::CTcpConnect(CNetWorkThread& inNetWorkThread, CMthNetEndpoint& epPeer)
  : socketClient(inNetWorkThread.GetIoService()), tNetWorkThread(inNetWorkThread),
    epPeer(epPeer), pCurSendNvBuf(NULL), nPostRecvOpCount(0), nRefCount(0)
{
    fInBound = false;
    fIfOpenSocket = true;
    ui64TcpConnNetId = CBaseUniqueId::CreateUniqueId(0, 1, inNetWorkThread.GetThreadId(), 0);
}

CTcpConnect::~CTcpConnect()
{
    if (fIfOpenSocket)
    {
        try
        {
            socketClient.close();
        }
        catch (exception& e)
        {
            StdError("TcpConnect", "Close socket error: %s", e.what());
        }
        fIfOpenSocket = false;
    }

    CMthNetPackData* pTempBuf;
    while (!qWaitSendQueue.empty())
    {
        pTempBuf = qWaitSendQueue.front();
        qWaitSendQueue.pop();
        if (pTempBuf)
        {
            delete pTempBuf;
        }
    }
    if (pCurSendNvBuf)
    {
        delete pCurSendNvBuf;
        pCurSendNvBuf = NULL;
    }
}

bool CTcpConnect::Accept()
{
    if (fIfOpenSocket && socketClient.is_open())
    {
        try
        {
            epPeer.SetEndpoint(socketClient.remote_endpoint());
        }
        catch (exception& e)
        {
            StdError("TcpConnect", "Accept: remote_endpoint error: %s", e.what());
            return false;
        }
        try
        {
            epLocal.SetEndpoint(socketClient.local_endpoint());
        }
        catch (exception& e)
        {
            StdError("TcpConnect", "Accept: local_endpoint error: %s", e.what());
            return false;
        }

        CMthNetPackData* pMthBuf = new CMthNetPackData(ui64TcpConnNetId, NET_MSG_TYPE_SETUP, NET_DIS_CAUSE_UNKNOWN, epPeer, epLinkListen);
        if (!tNetWorkThread.qRecvQueue.SetData(pMthBuf, 4000))
        {
            StdError("TcpConnect", "Accept: SetData fail");
            delete pMthBuf;
            return false;
        }
        PostRecvRequest();
        return true;
    }
    return false;
}

void CTcpConnect::TcpRemove(E_DISCONNECT_CAUSE eCloseCause)
{
    if (fIfOpenSocket)
    {
        try
        {
            socketClient.close();
        }
        catch (exception& e)
        {
            StdError("TcpConnect", "TcpRemove: Close socket error: %s", e.what());
        }
        tNetWorkThread.Disconnect(this);
        CMthNetPackData* pMthBuf = new CMthNetPackData(ui64TcpConnNetId, NET_MSG_TYPE_CLOSE, eCloseCause, epPeer, epLinkListen);
        if (!tNetWorkThread.qRecvQueue.SetData(pMthBuf, 4000))
        {
            StdError("TcpConnect", "TcpRemove: SetData fail");
            delete pMthBuf;
        }
        fIfOpenSocket = false;
        tmCloseTime = time(NULL);
    }

    if (nRefCount == 0)
    {
        tNetWorkThread.RemoveTcpConnect(this);
        delete this;
    }
}

void CTcpConnect::SetSendData(CMthNetPackData* pNvBuf)
{
    if (pNvBuf)
    {
        if (pNvBuf->GetDataBuf() && pNvBuf->GetDataLen() > 0)
        {
            if (pCurSendNvBuf == NULL)
            {
                pCurSendNvBuf = pNvBuf;
                PostSendRequest();
            }
            else
            {
                qWaitSendQueue.push(pNvBuf);
            }
        }
        else
        {
            delete pNvBuf;
        }
    }
}

void CTcpConnect::PostRecvRequest()
{
    if (nPostRecvOpCount == 0)
    {
        socketClient.async_read_some(boost::asio::buffer(buffer_read, sizeof(buffer_read)),
                                     boost::bind(&CTcpConnect::HandleRead, this,
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred));
        nPostRecvOpCount++;
        nRefCount++;
    }
}

void CTcpConnect::PostSendRequest()
{
    if (pCurSendNvBuf)
    {
        boost::asio::async_write(socketClient, boost::asio::buffer(pCurSendNvBuf->GetDataBuf(), pCurSendNvBuf->GetDataLen()),
                                 boost::bind(&CTcpConnect::HandleWrite, this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
        nRefCount++;
    }
}

bool CTcpConnect::ConnectCompleted()
{
    if (!socketClient.is_open())
    {
        StdError("TcpConnect", "ConnectCompleted: socket not open");
        return false;
    }
    try
    {
        epLocal.SetEndpoint(socketClient.local_endpoint());
    }
    catch (exception& e)
    {
        StdError("TcpConnect", "ConnectCompleted: local_endpoint error: %s", e.what());
        return false;
    }
    CMthNetPackData* pMthBuf = new CMthNetPackData(ui64TcpConnNetId, NET_MSG_TYPE_COMPLETE_NOTIFY, NET_DIS_CAUSE_CONNECT_SUCCESS, epPeer, epLocal);
    if (!tNetWorkThread.qRecvQueue.SetData(pMthBuf, 4000))
    {
        StdError("TcpConnect", "ConnectCompleted: SetData fail");
        delete pMthBuf;
        return false;
    }
    PostRecvRequest();
    return true;
}

void CTcpConnect::ConnectFail()
{
    CMthNetPackData* pMthBuf = new CMthNetPackData(ui64TcpConnNetId, NET_MSG_TYPE_COMPLETE_NOTIFY, NET_DIS_CAUSE_CONNECT_FAIL, epPeer, epLocal);
    if (!tNetWorkThread.qRecvQueue.SetData(pMthBuf, 4000))
    {
        StdError("TcpConnect", "ConnectFail: SetData fail");
        delete pMthBuf;
    }
}

void CTcpConnect::DoRemoveTimer()
{
    if (!fIfOpenSocket && time(NULL) - tmCloseTime > 5)
    {
        tNetWorkThread.RemoveTcpConnect(this);
        delete this;
    }
}

//---------------------------------------------------------------------------------------------------
void CTcpConnect::HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    nRefCount--;
    if (nPostRecvOpCount > 0)
    {
        nPostRecvOpCount--;
    }

    if (!ec)
    {
        bool fLimitRecv = false;
        CMthNetPackData* pMthBuf = new CMthNetPackData(ui64TcpConnNetId, NET_MSG_TYPE_DATA, NET_DIS_CAUSE_UNKNOWN, epPeer, epLinkListen, buffer_read, bytes_transferred);
        if (!tNetWorkThread.qRecvQueue.PushPacket(pMthBuf, fLimitRecv, 4000))
        {
            StdError("TcpConnect", "HandleRead: PushPacket fail");
            delete pMthBuf;
            TcpRemove(NET_DIS_CAUSE_LOCAL_CLOSE);
            return;
        }
        if (fLimitRecv)
        {
            return;
        }
        PostRecvRequest();
    }
    else
    {
        TcpRemove(NET_DIS_CAUSE_PEER_CLOSE);
    }
}

void CTcpConnect::HandleWrite(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
    nRefCount--;
    if (!ec)
    {
        if (pCurSendNvBuf)
        {
            delete pCurSendNvBuf;
            pCurSendNvBuf = NULL;
        }
        while (!qWaitSendQueue.empty())
        {
            pCurSendNvBuf = qWaitSendQueue.front();
            qWaitSendQueue.pop();
            if (pCurSendNvBuf && pCurSendNvBuf->GetDataBuf() && pCurSendNvBuf->GetDataLen() > 0)
            {
                PostSendRequest();
                break;
            }
            else
            {
                if (pCurSendNvBuf)
                {
                    delete pCurSendNvBuf;
                    pCurSendNvBuf = NULL;
                }
            }
        }
    }
    else
    {
        TcpRemove(NET_DIS_CAUSE_PEER_CLOSE);
    }
}

} // namespace network
