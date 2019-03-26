// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netmsgwork.h"

namespace dnseed
{


//---------------------------------------------------------------------------
CWorkThreadPool::CWorkThreadPool(CDnseedConfig *pCfg, CNetWorkService *nws,CBbAddrPool *pBbAddrPoolIn) : 
    pDNSeedCfg(pCfg), pNetWorkService(nws), pBbAddrPool(pBbAddrPoolIn)
{
    nWorkThreadCount = pNetWorkService->GetWorkThreadCount();
    if (nWorkThreadCount > MAX_NET_WORK_THREAD_COUNT)
    {
        nWorkThreadCount = MAX_NET_WORK_THREAD_COUNT;
    }

    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        pWorkThreadTable[i] = new CMsgWorkThread(nWorkThreadCount, i, pCfg, nws, pBbAddrPoolIn);
    }
}

CWorkThreadPool::~CWorkThreadPool()
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        delete pWorkThreadTable[i];
        pWorkThreadTable[i] = NULL;
    }
}

bool CWorkThreadPool::StartAll()
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        if (pWorkThreadTable[i] == NULL || !pWorkThreadTable[i]->Start())
        {
            StopAll();
            return false;
        }
    }
    return true;
}

void CWorkThreadPool::StopAll()
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        if (pWorkThreadTable[i])
        {
            pWorkThreadTable[i]->Stop();
        }
    }
}

bool CWorkThreadPool::StatRunData(CRunStatData& tStatData)
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        if (pWorkThreadTable[i])
        {
            CRunStatData tTempStat;
            pWorkThreadTable[i]->GetNetStatData(tTempStat);
            tStatData += tTempStat;
        }
    }

    return true;
}


//-----------------------------------------------------------------------------------------------
CMsgWorkThread::CMsgWorkThread(uint32 nThreadCount, uint32 nThreadIndex, CDnseedConfig *pCfg, CNetWorkService *nws, CBbAddrPool *pBbAddrPoolIn) : 
    nWorkThreadCount(nThreadCount), nWorkThreadIndex(nThreadIndex), pDNSeedCfg(pCfg), pNetWorkService(nws), pBbAddrPool(pBbAddrPoolIn), 
    pThreadMsgWork(NULL),pNetDataQueue(NULL),fRunFlag(false),tmPrevTimerDoTime(0)
{
    pNetDataQueue = &(nws->GetWorkRecvQueue(nThreadIndex));

    if (nThreadCount > 1)
    {
        if (pCfg->nBackTestAddrCount < nThreadCount)
        {
            nPersCalloutAddrCount = 1;
        }
        else
        {
            nPersCalloutAddrCount = pCfg->nBackTestAddrCount / nThreadCount;
            if (nThreadIndex == nThreadCount - 1)
            {
                nPersCalloutAddrCount += pCfg->nBackTestAddrCount % nThreadCount;
            }
        }
    }
    else
    {
        nPersCalloutAddrCount = pCfg->nBackTestAddrCount;
    }

    nStartBackTestTime = 0;
    nCompBackTestCount = 0;
}

CMsgWorkThread::~CMsgWorkThread()
{
    Stop();
}

bool CMsgWorkThread::Start()
{
    fRunFlag = true;
    pThreadMsgWork = new boost::thread(boost::bind(&CMsgWorkThread::Work, this));
    if (pThreadMsgWork == NULL)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Error msg work thread start.");
        return false;
    }
    return true;
}

void CMsgWorkThread::Stop()
{
    if (pThreadMsgWork)
    {
        fRunFlag = false;
        pThreadMsgWork->join();

        delete pThreadMsgWork;
        pThreadMsgWork = NULL;
    }
}

void CMsgWorkThread::GetNetStatData(CRunStatData& tStatOut)
{
    boost::unique_lock<boost::shared_mutex> lock(lockStat);
    tStatOut = tNetStatData;
}


//-------------------------------------------------------------------------------
void CMsgWorkThread::Work()
{
    nStartBackTestTime = GetTimeMillis();
    
    while (fRunFlag)
    {
        Timer();

        CMthNetPackData *pPackData = NULL;
        if (!pNetDataQueue->GetData(pPackData, 100) || pPackData == NULL)
        {
            continue;
        }
        DoPacket(pPackData);
        delete pPackData;
    }
}

void CMsgWorkThread::Timer()
{
    time_t tmCurTime = time(NULL);
    if (tmCurTime - tmPrevTimerDoTime >= 1 || tmCurTime < tmPrevTimerDoTime)
    {
        tmPrevTimerDoTime = tmCurTime;
        DoPeerState(tmCurTime);
    }
    DoCallConnect();
}


//-------------------------------------------------------------------------------
void CMsgWorkThread::DoPacket(CMthNetPackData *pPackData)
{
    switch (pPackData->eMsgType)
    {
    case NET_MSG_TYPE_SETUP:
    {
        if (STD_DEBUG)
        {
            string sInfo = string("Setup message, peer: ") + pPackData->epPeerAddr.ToString();
            blockhead::StdDebug("CFLOW", sInfo.c_str());
        }
        if (AddNetPeer(true,pPackData->ui64NetId,pPackData->epPeerAddr,pPackData->epLocalAddr) == NULL)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "DoPacket NET_MSG_TYPE_SETUP AddNetPeer error.");
            pNetWorkService->RemoveNetPort(pPackData->ui64NetId);
            break;
        }
        break;
    }
    case NET_MSG_TYPE_DATA:
    {
        CNetPeer *pNetPeer = GetNetPeer(pPackData->ui64NetId);
        if (pNetPeer)
        {
            if (!pNetPeer->DoRecvPacket(pPackData))
            {
                ActiveClosePeer(pPackData->ui64NetId);
                break;
            }
        }
        break;
    }
    case NET_MSG_TYPE_CLOSE:
    {
        if (STD_DEBUG)
        {
            switch (pPackData->eDisCause)
            {
            case NET_DIS_CAUSE_UNKNOWN:
                blockhead::StdDebug("CFLOW", "Close message: Unknown.");
                break;
            case NET_DIS_CAUSE_PEER_CLOSE:
                blockhead::StdDebug("CFLOW", "Close message: Peer close.");
                break;
            case NET_DIS_CAUSE_LOCAL_CLOSE:
                blockhead::StdDebug("CFLOW", "Close message: Local close.");
                break;
            case NET_DIS_CAUSE_CONNECT_FAIL:
                blockhead::StdDebug("CFLOW", "Close message: Connect fail.");
                break;
            default:
                blockhead::StdDebug("CFLOW", "Close message: Other.");
                break;
            }
        }
        DelNetPeer(pPackData->ui64NetId);
        break;
    }
    case NET_MSG_TYPE_COMPLETE_NOTIFY:
    {
        if (pPackData->eDisCause != NET_DIS_CAUSE_CONNECT_SUCCESS)
        {
            if (STD_DEBUG)
            {
                string sErrorInfo = string("Connect peer fail, peer: ") + pPackData->epPeerAddr.ToString();
                blockhead::StdDebug("CFLOW", sErrorInfo.c_str());
            }

            {
                boost::unique_lock<boost::shared_mutex> lock(lockStat);
                tNetStatData.nTotalOutCount++;
                tNetStatData.nTotalOutFailCount++;
            }
            break;
        }
        if (STD_DEBUG)
        {
            string sInfo = string("Connect peer success, peer: ") + pPackData->epPeerAddr.ToString();
            blockhead::StdDebug("CFLOW", sInfo.c_str());
        }

        CNetPeer* pNetPeer = AddNetPeer(false, pPackData->ui64NetId, pPackData->epPeerAddr, pPackData->epLocalAddr);
        if (pNetPeer == NULL)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "DoPacket NET_MSG_TYPE_COMPLETE_NOTIFY AddNetPeer error.");
            ActiveClosePeer(pPackData->ui64NetId);
            break;
        }
        if (!pNetPeer->DoOutBoundConnectSuccess(pPackData->epLocalAddr))
        {
            ActiveClosePeer(pPackData->ui64NetId);
            break;
        }
        break;
    }
    }
}

bool CMsgWorkThread::SendDataPacket(uint64 nNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, char *pBuf, uint32 nLen)
{
    if (nNetId == 0 || pBuf == NULL || nLen == 0)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Param error.");
        return false;
    }
    CMthNetPackData *pPackData = new CMthNetPackData(nNetId, NET_MSG_TYPE_DATA, NET_DIS_CAUSE_UNKNOWN, tPeerEpIn, tLocalEpIn, pBuf, nLen);
    return pNetWorkService->ReqSendData(pPackData);
}

CNetPeer* CMsgWorkThread::AddNetPeer(bool fInBoundIn, uint64 nPeerNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn)
{
    if (GetNetPeer(nPeerNetId) == NULL)
    {
        CNetPeer *pNetPeer = new CNetPeer(this, pBbAddrPool, pDNSeedCfg->nMagicNum, 
            fInBoundIn, nPeerNetId, tPeerEpIn, tLocalEpIn, pDNSeedCfg->fAllowAllAddr);
        if (pNetPeer == NULL)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "new CNetPeer fail.");
            return NULL;
        }
        if (mapPeer.insert(make_pair(nPeerNetId,pNetPeer)).second)
        {
            boost::unique_lock<boost::shared_mutex> lock(lockStat);
            if (fInBoundIn)
            {
                tNetStatData.nInBoundCount++;
            }
            else
            {
                tNetStatData.nOutBoundCount++;
                tNetStatData.nTotalOutSuccessCount++;
            }
            tNetStatData.nTcpConnCount++;
            return pNetPeer;
        }
        else
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "mapPeer insert fail.");
        }
    }
    return NULL;
}

void CMsgWorkThread::DelNetPeer(uint64 nPeerNetId)
{
    map<uint64, CNetPeer*>::iterator it;
    it = mapPeer.find(nPeerNetId);
    if (it != mapPeer.end())
    {
        CNetPeer *pPeer = it->second;
        mapPeer.erase(it);
        if (pPeer)
        {
            {
                boost::unique_lock<boost::shared_mutex> lock(lockStat);
                if (pPeer->fInBound)
                {
                    tNetStatData.nInBoundCount--;
                    tNetStatData.nTotalInCount++;
                }
                else
                {
                    tNetStatData.nOutBoundCount--;
                    tNetStatData.nTotalOutCount++;
                }
                tNetStatData.nTcpConnCount--;

                if (pPeer->fIfDoComplete)
                {
                    if (pPeer->fInBound)
                    {
                        tNetStatData.nTotalInWorkSuccessCount++;
                    }
                    else
                    {
                        tNetStatData.nTotalOutWorkSuccessCount++;
                    }
                }
                else
                {
                    if (pPeer->fInBound)
                    {
                        tNetStatData.nTotalInWorkFailCount++;
                    }
                    else
                    {
                        tNetStatData.nTotalOutWorkFailCount++;
                    }
                }
            }
            delete pPeer;
        }
    }
}

CNetPeer* CMsgWorkThread::GetNetPeer(uint64 nPeerNetId)
{
    map<uint64, CNetPeer*>::iterator it = mapPeer.find(nPeerNetId);
    if (it != mapPeer.end())
    {
        return it->second;
    }
    return NULL;
}

void CMsgWorkThread::ActiveClosePeer(uint64 nPeerNetId)
{
    blockhead::StdDebug("CFLOW", "Active close connect");
    pNetWorkService->RemoveNetPort(nPeerNetId);
    DelNetPeer(nPeerNetId);
}

bool CMsgWorkThread::HandlePeerHandshaked(CNetPeer *pPeer)
{
    tcp::endpoint ep;
    pPeer->tPeerEp.GetEndpoint(ep);
    pBbAddrPool->UpdateNetTime(ep.address(), pPeer->nTimeDelta);
    pBbAddrPool->UpdateHeight(pPeer->tPeerEp, pPeer->nStartingHeight);
    return true;
}

void CMsgWorkThread::DoCallConnect()
{
    int64 nTestTimeLen = GetTimeMillis() - nStartBackTestTime;
    uint64 nNeedTestCount = nPersCalloutAddrCount * nTestTimeLen / 1000 - nCompBackTestCount;
    if (nNeedTestCount <= 0)
    {
        return ;
    }

    vector<CBbAddr> vBbAddr;
    if (!pBbAddrPool->GetCallConnectAddrList(vBbAddr, nNeedTestCount, pDNSeedCfg->fStressBackTest) || vBbAddr.size() == 0)
    {
        nStartBackTestTime = GetTimeMillis();
        nCompBackTestCount = 0;
        return;
    }
    if (vBbAddr.size() < nNeedTestCount)
    {
        nStartBackTestTime = GetTimeMillis();
        nCompBackTestCount = 0;
    }

    vector<CBbAddr>::iterator it;
    for (it = vBbAddr.begin(); it != vBbAddr.end(); it++)
    {
        StartConnectPeer(*it);
        nCompBackTestCount++;
    }
}

void CMsgWorkThread::DoPeerState(time_t tmCurTime)
{
    CNetPeer* pNetPeer;
    uint32 nInCount = 0, nOutCount = 0;

    map<uint64, CNetPeer*>::iterator it;
    for (it = mapPeer.begin(); it != mapPeer.end(); )
    {
        if (it->second)
        {
            pNetPeer = it->second;
            it++;

            if (pNetPeer->fInBound) nInCount++;
            else nOutCount++;

            if(!pNetPeer->DoStateTimer(tmCurTime))
            {
                ActiveClosePeer(pNetPeer->nPeerNetId);
            }
        }
        else
        {
            it++;
        }
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(lockStat);
        tNetStatData.nInBoundCount = nInCount;
        tNetStatData.nOutBoundCount = nOutCount;
        tNetStatData.nTcpConnCount = nInCount + nOutCount;
    }
}

void CMsgWorkThread::StartConnectPeer(CBbAddr& tBbAddr)
{
    if (STD_DEBUG)
    {
        string sInfo = string("Start connect peer, peer: ") + tBbAddr.GetEp().ToString();
        blockhead::StdDebug("CFLOW", sInfo.c_str());
    }

    uint64 nOutNetId = 0;
    if (!pNetWorkService->ReqTcpConnect(tBbAddr.GetEp(), nOutNetId))
    {
        string sInfo = string("ReqTcpConnect fail, peer: ") + tBbAddr.GetEp().ToString();
        blockhead::StdError(__PRETTY_FUNCTION__, sInfo.c_str());
        return;
    }
}

}  // namespace dnseed

