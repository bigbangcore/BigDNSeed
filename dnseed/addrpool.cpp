// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addrpool.h"

#include "dbstorage.h"

using namespace std;
using namespace blockhead;

namespace dnseed
{

//-------------------------------------------------------------------------
CBbAddr::CBbAddr()
{
    nService = 0;
    iScore = 0;
    fConfidentAddr = false;
    nStartingHeight = 0;
}

bool CBbAddr::SetBbAddr(string& strIp, uint16 nPort, uint64 nServiceIn, int iScoreIn)
{
    if (!tNetEp.SetAddrPort(strIp, nPort))
    {
        return false;
    }
    fConfidentAddr = false;
    nService = nServiceIn;
    iScore = iScoreIn;
    return true;
}

bool CBbAddr::SetBbAddr(tcp::endpoint& ep, uint64 nServiceIn, int iScoreIn)
{
    if (!tNetEp.SetAddrPort(ep))
    {
        return false;
    }
    fConfidentAddr = false;
    nService = nServiceIn;
    iScore = iScoreIn;
    return true;
}

bool CBbAddr::GetAddress(CAddress& tAddr)
{
    tcp::endpoint ep;
    if (!tNetEp.GetEndpoint(ep))
    {
        return false;
    }
    tAddr.SetAddress(nService, ep);
    return true;
}

bool CBbAddr::GetAddress(tcp::endpoint& ep, uint64& nServiceOut, int& iScoreOut)
{
    if (!tNetEp.GetEndpoint(ep))
    {
        return false;
    }
    nServiceOut = nService;
    iScoreOut = iScore;
    return true;
}

void CBbAddr::DoScore(int iDoValue)
{
    iScore += iDoValue;
    if (iScore < NMS_ATP_MIN_ADDR_SCORE)
    {
        iScore = NMS_ATP_MIN_ADDR_SCORE;
    }
    else if (iScore > NMS_ATP_MAX_ADDR_SCORE)
    {
        iScore = NMS_ATP_MAX_ADDR_SCORE;
    }
}

void CBbAddr::DoScoreByHeight(int nRefHeight)
{
    if (fConfidentAddr)
    {
        iScore = NMS_ATP_MAX_ADDR_SCORE;
        return;
    }
    int iDiff = abs(nStartingHeight - nRefHeight);
    if (iDiff <= NMS_ATP_HEIGHT_DIFF_RANGE)
    {
        if (iDiff == 0)
        {
            DoScore(10);
        }
        else
        {
            DoScore(iDiff * 10 / NMS_ATP_HEIGHT_DIFF_RANGE);
        }
    }
    else
    {
        if (iDiff >= NMS_ATP_HEIGHT_DIFF_RANGE * 10)
        {
            DoScore(-10);
        }
        else
        {
            iDiff = (iDiff - NMS_ATP_HEIGHT_DIFF_RANGE) / 10;
            DoScore(-(iDiff * 10 / NMS_ATP_HEIGHT_DIFF_RANGE));
        }
    }
}

//-------------------------------------------------------------------------
CBbAddrPool::CBbAddrPool(CDnseedConfig* pCfg)
  : pDnseedCfg(pCfg), pDbStorage(NULL)
{
}

CBbAddrPool::~CBbAddrPool()
{
    ReleaseAddrPool();
}

void CBbAddrPool::SetDbStorage(CDbStorage* pDbs)
{
    pDbStorage = pDbs;
}

bool CBbAddrPool::SetTrustAddr(set<string>& setTrustAddrIn)
{
    for (auto addr : setTrustAddrIn)
    {
        AddConfidentAddr(addr);
    }
    return true;
}

bool CBbAddrPool::AddConfidentAddr(string& sAddr)
{
    CMthNetEndpoint ep;
    if (!ep.SetAddrPort(sAddr))
    {
        string sErrorInfo = string("Error confident addr: ") + sAddr;
        blockhead::StdError(__PRETTY_FUNCTION__, sErrorInfo.c_str());
        return false;
    }
    string strAddr = ep.ToString();

    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pBbAddr = GetAddrNoLock(strAddr);
    if (pBbAddr == NULL)
    {
        CBbAddr* pBbAddr = new CBbAddr();
        pBbAddr->tNetEp = ep;
        pBbAddr->fConfidentAddr = true;
        pBbAddr->nService = NODE_NETWORK;
        pBbAddr->iScore = NMS_ATP_MAX_ADDR_SCORE;
        pBbAddr->tTestParam.nNextConnIntervalTime = 1;
        if (!mapAddrPool.insert(make_pair(strAddr, pBbAddr)).second)
        {
            string sErrorInfo = string("Error mapAddrPool insert, addr: ") + sAddr;
            blockhead::StdError(__PRETTY_FUNCTION__, sErrorInfo.c_str());
            delete pBbAddr;
            return false;
        }
        if (setConfidentAddrPool.count(strAddr) == 0)
        {
            setConfidentAddrPool.insert(strAddr);
        }
        /*Confident address not write db*/
    }
    else
    {
        pBbAddr->fConfidentAddr = true;
        if (setConfidentAddrPool.count(strAddr) == 0)
        {
            setConfidentAddrPool.insert(strAddr);
        }
    }
    return true;
}

bool CBbAddrPool::AddAddrFromDb(CMthNetEndpoint& ep, uint64 nService, int iScore)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    string strAddr = ep.ToString();
    CBbAddr* pBbAddr = GetAddrNoLock(strAddr);
    if (pBbAddr == NULL)
    {
        CBbAddr* pBbAddr = new CBbAddr();
        pBbAddr->tNetEp = ep;
        pBbAddr->nService = nService;
        pBbAddr->iScore = iScore;
        if (!mapAddrPool.insert(make_pair(strAddr, pBbAddr)).second)
        {
            delete pBbAddr;
            return false;
        }
    }
    return true;
}

bool CBbAddrPool::AddRecvAddr(tcp::endpoint& ep, uint64 nServiceIn)
{
    CMthNetEndpoint tEp;
    if (!tEp.SetAddrPort(ep))
    {
        return false;
    }
    string strAddr = tEp.ToString();

    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pBbAddr = GetAddrNoLock(strAddr);
    if (pBbAddr == NULL)
    {
        CBbAddr* pBbAddr = new CBbAddr();
        pBbAddr->tNetEp = tEp;
        pBbAddr->nService = nServiceIn;
        if (!mapAddrPool.insert(make_pair(strAddr, pBbAddr)).second)
        {
            delete pBbAddr;
            return false;
        }
        if (pDbStorage)
        {
            CDNSeedNode* pNode = new CDNSeedNode(DDN_E_MSG_TYPE_INSERT, pBbAddr->GetEp().GetIp(), pBbAddr->GetEp().GetPort(),
                                                 pBbAddr->GetService(), pBbAddr->GetScore());
            lock.unlock();
            if (pNode)
            {
                pDbStorage->PostDbMessage(pNode);
            }
        }
    }
    else
    {
        if (pBbAddr->nService != nServiceIn)
        {
            pBbAddr->nService = nServiceIn;
            if (pDbStorage)
            {
                CDNSeedNode* pNode = new CDNSeedNode(DDN_E_MSG_TYPE_UPDATE, pBbAddr->GetEp().GetIp(), pBbAddr->GetEp().GetPort(),
                                                     pBbAddr->GetService(), pBbAddr->GetScore());
                lock.unlock();
                if (pNode)
                {
                    pDbStorage->PostDbMessage(pNode);
                }
            }
        }
    }
    return true;
}

void CBbAddrPool::DelAddr(CMthNetEndpoint& ep)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    map<string, CBbAddr*>::iterator it;
    it = mapAddrPool.find(ep.ToString());
    if (it != mapAddrPool.end())
    {
        CBbAddr* pNode = (*it).second;
        if (pNode->fConfidentAddr)
        {
            setConfidentAddrPool.erase(it->first);
        }
        mapAddrPool.erase(it);
        if (pDbStorage)
        {
            CDNSeedNode* pDbMsg = new CDNSeedNode(DDN_E_MSG_TYPE_DELETE, ep.GetIp(), ep.GetPort(), pNode->GetService(), pNode->GetScore());
            lock.unlock();
            if (pDbMsg)
            {
                pDbStorage->PostDbMessage(pDbMsg);
            }
        }
        if (pNode)
        {
            delete pNode;
        }
    }
}

void CBbAddrPool::DelAddr(CBbAddr& addr)
{
    DelAddr(addr.GetEp());
}

bool CBbAddrPool::QueryAddr(CMthNetEndpoint& ep, CBbAddr& addr)
{
    boost::shared_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pBbAddr = GetAddrNoLock(ep.ToString());
    if (pBbAddr)
    {
        addr = *pBbAddr;
        return true;
    }
    return false;
}

bool CBbAddrPool::DoScore(CMthNetEndpoint& ep, int iDoValue)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pBbAddr = GetAddrNoLock(ep.ToString());
    if (pBbAddr)
    {
        pBbAddr->DoScore(iDoValue);
        if (pDbStorage)
        {
            CDNSeedNode* pNode = new CDNSeedNode(DDN_E_MSG_TYPE_DELETE, ep.GetIp(), ep.GetPort(), pBbAddr->GetService(), pBbAddr->GetScore());
            lock.unlock();
            if (pNode)
            {
                pDbStorage->PostDbMessage(pNode);
            }
            return true;
        }
    }
    return false;
}

bool CBbAddrPool::DoScore(CBbAddr& addr, int iDoValue)
{
    return DoScore(addr.GetEp(), iDoValue);
}

bool CBbAddrPool::UpdateHeight(CMthNetEndpoint& ep, int iHeight)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pBbAddr = GetAddrNoLock(ep.ToString());
    if (pBbAddr)
    {
        int iOldScore = pBbAddr->GetScore();
        pBbAddr->nStartingHeight = iHeight;
        pBbAddr->DoScoreByHeight(GetConfidentHeight());
        if (pBbAddr->fConfidentAddr)
        {
            uint64 nTotalHeight = 0;
            uint32 nNodeCount = 0;
            CBbAddr* pBbConAddr = NULL;
            set<string>::iterator it;
            for (it = setConfidentAddrPool.begin(); it != setConfidentAddrPool.end(); it++)
            {
                pBbConAddr = GetAddrNoLock(*it);
                if (pBbConAddr && pBbConAddr->fConfidentAddr && pBbConAddr->nStartingHeight > 0)
                {
                    nTotalHeight += pBbConAddr->nStartingHeight;
                    nNodeCount++;
                }
            }
            if (nNodeCount > 0)
            {
                SetConfidentHeight((int)(nTotalHeight / nNodeCount));
            }
        }
        else
        {
            if (iOldScore != pBbAddr->GetScore() && pDbStorage)
            {
                CDNSeedNode* pNode = new CDNSeedNode(DDN_E_MSG_TYPE_UPDATE, pBbAddr->GetEp().GetIp(), pBbAddr->GetEp().GetPort(),
                                                     pBbAddr->GetService(), pBbAddr->GetScore());
                lock.unlock();
                if (pNode)
                {
                    pDbStorage->PostDbMessage(pNode);
                }
            }
        }
        return true;
    }
    return false;
}

void CBbAddrPool::ReleaseAddrPool()
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);
    CBbAddr* pNode;
    map<string, CBbAddr*>::iterator it;
    for (it = mapAddrPool.begin(); it != mapAddrPool.end();)
    {
        pNode = (*it).second;
        mapAddrPool.erase(it++);
        if (pNode)
        {
            delete pNode;
        }
    }
}

CBbAddr* CBbAddrPool::GetAddrNoLock(const string& sAddrPort)
{
    map<string, CBbAddr*>::iterator it = mapAddrPool.find(sAddrPort);
    if (it != mapAddrPool.end())
    {
        return (*it).second;
    }
    return NULL;
}

void CBbAddrPool::GetGoodAddressList(vector<CAddress>& vAddrList)
{
    boost::shared_lock<boost::shared_mutex> lock(lockAddrPool);

    tcp::endpoint ep;
    uint64 nService;
    int iScore;
    uint32 nMapSize;
    uint32 nGetPos;
    uint32 nGoodCount;
    CBbAddr* pBbAddr;
    CBbAddr** ppGoodBbAddrTable;
    map<string, CBbAddr*>::iterator it;

    //<Need improvement>
    nMapSize = mapAddrPool.size();
    if (nMapSize == 0)
    {
        return;
    }
    ppGoodBbAddrTable = (CBbAddr**)malloc(sizeof(CBbAddr*) * nMapSize);
    if (ppGoodBbAddrTable == NULL)
    {
        return;
    }
    memset(ppGoodBbAddrTable, 0, sizeof(CBbAddr*) * nMapSize);

    nGoodCount = 0;
    for (it = mapAddrPool.begin(); it != mapAddrPool.end(); it++)
    {
        pBbAddr = it->second;
        if (pBbAddr && pBbAddr->iScore >= pDnseedCfg->nGoodAddrScore)
        {
            ppGoodBbAddrTable[nGoodCount++] = pBbAddr;
        }
    }

    srand(time(NULL) + GetTimeMillis());
    for (int i = 0; i < nGoodCount * 10; i++)
    {
        if (nGoodCount <= pDnseedCfg->nGetGoodAddrCount)
        {
            if (i >= nGoodCount)
            {
                break;
            }
            nGetPos = i;
        }
        else
        {
            nGetPos = rand() % nGoodCount;
        }
        pBbAddr = ppGoodBbAddrTable[nGetPos];
        if (pBbAddr)
        {
            if (pBbAddr->GetAddress(ep, nService, iScore))
            {
                vAddrList.push_back(CAddress(nService, ep));
                if (vAddrList.size() >= pDnseedCfg->nGetGoodAddrCount)
                {
                    break;
                }
            }
            ppGoodBbAddrTable[nGetPos] = NULL;
        }
    }
    if (vAddrList.size() < pDnseedCfg->nGetGoodAddrCount && nGoodCount > pDnseedCfg->nGetGoodAddrCount)
    {
        for (nGetPos = 0; nGetPos < nGoodCount; nGetPos++)
        {
            pBbAddr = ppGoodBbAddrTable[nGetPos];
            if (pBbAddr && pBbAddr->GetAddress(ep, nService, iScore))
            {
                vAddrList.push_back(CAddress(nService, ep));
                if (vAddrList.size() >= pDnseedCfg->nGetGoodAddrCount)
                {
                    break;
                }
            }
        }
    }

    free(ppGoodBbAddrTable);
}

bool CBbAddrPool::GetCallConnectAddrList(vector<CBbAddr>& vBbAddr, uint32 nGetAddrCount, bool fStressTest)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);

    int64 nCurTime;
    map<string, CBbAddr*>::iterator it;
    uint32 nCheckCount;
    CBbAddr* pBbAddr = NULL;

    //<Need improvement>
    nCurTime = GetTime();
    nCheckCount = mapAddrPool.size();
    if (nCheckCount == 0)
    {
        sPrevTestAddr.clear();
        return false;
    }

    if (sPrevTestAddr.empty())
    {
        it = mapAddrPool.begin();
    }
    else
    {
        it = mapAddrPool.find(sPrevTestAddr);
    }
    for (; nCheckCount > 0; it++, nCheckCount--)
    {
        if (it == mapAddrPool.end())
        {
            it = mapAddrPool.begin();
            if (it == mapAddrPool.end())
            {
                break;
            }
        }
        pBbAddr = it->second;
        if (pBbAddr && nCurTime - pBbAddr->tTestParam.nPrevConnectTime >= pBbAddr->tTestParam.nNextConnIntervalTime)
        {
            pBbAddr->tTestParam.nPrevConnectTime = nCurTime;
            pBbAddr->tTestParam.nConnectCount++;

            if (!fStressTest)
            {
                if (pBbAddr->tTestParam.nNextConnIntervalTime < NMS_ATP_TEST_CONN_INTERVAL_MAX_TIME)
                {
                    if (pBbAddr->iScore < NMS_ATP_GOOD_ADDR_SCORE)
                    {
                        if (pBbAddr->tTestParam.nNextConnIntervalTime < NMS_ATP_TEST_CONN_INTERVAL_START_TIME)
                        {
                            pBbAddr->tTestParam.nNextConnIntervalTime = NMS_ATP_TEST_CONN_INTERVAL_START_TIME;
                        }
                        else
                        {
                            pBbAddr->tTestParam.nNextConnIntervalTime += 10;
                        }
                    }
                    else
                    {
                        pBbAddr->tTestParam.nNextConnIntervalTime += 120;
                    }
                    if (pBbAddr->tTestParam.nNextConnIntervalTime > NMS_ATP_TEST_CONN_INTERVAL_MAX_TIME)
                    {
                        pBbAddr->tTestParam.nNextConnIntervalTime = NMS_ATP_TEST_CONN_INTERVAL_MAX_TIME;
                    }
                }
            }
            else
            {
                pBbAddr->tTestParam.nNextConnIntervalTime = 1;
            }

            vBbAddr.push_back(CBbAddr(*pBbAddr));
            if (vBbAddr.size() >= nGetAddrCount)
            {
                break;
            }
        }
    }

    if (pBbAddr)
    {
        sPrevTestAddr = pBbAddr->tNetEp.ToString();
    }
    else
    {
        it = mapAddrPool.begin();
        if (it != mapAddrPool.end() && it->second)
        {
            sPrevTestAddr = it->second->tNetEp.ToString();
        }
        else
        {
            sPrevTestAddr.clear();
        }
    }

    return true;
}

int64 CBbAddrPool::GetNetTime()
{
    boost::shared_lock<boost::shared_mutex> lock(lockNetTime);
    return (GetTime() + tmNet.GetTimeOffset());
}

bool CBbAddrPool::UpdateNetTime(const boost::asio::ip::address& address, int64 nTimeDelta)
{
    boost::unique_lock<boost::shared_mutex> lock(lockNetTime);
    if (!tmNet.AddNew(address, nTimeDelta))
    {
        return false;
    }
    return true;
}

void CBbAddrPool::SetConfidentHeight(uint32 nHeight)
{
    boost::unique_lock<boost::shared_mutex> lock(lockConfidentAddr);
    nConfidentHeight = nHeight;
}

uint32 CBbAddrPool::GetConfidentHeight()
{
    boost::shared_lock<boost::shared_mutex> lock(lockConfidentAddr);
    return nConfidentHeight;
}

const uint256& CBbAddrPool::GetGenesisBlockHash()
{
    return pDnseedCfg->hashGenesisBlock;
}

} // namespace dnseed
