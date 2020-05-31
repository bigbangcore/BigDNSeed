// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dispatcher.h"

namespace dnseed
{

//-------------------------------------------------------------------------
CDispatcher::CDispatcher()
{
    pDnseedCfg = NULL;
    pNetWorkService = NULL;
    pWorkThreadPool = NULL;
    pDbStorage = NULL;
    pBbAddrPool = NULL;
    tmPrevStatTime = 0;
}

CDispatcher::~CDispatcher()
{
    StopService();
    Deinitialize();
}

bool CDispatcher::Initialize(CDnseedConfig* pCfg)
{
    pDnseedCfg = pCfg;

    pNetWorkService = new CNetWorkService(pCfg->nWorkThreadCount);
    if (pNetWorkService == NULL)
    {
        return false;
    }

    pBbAddrPool = new CBbAddrPool(pDnseedCfg);
    if (pBbAddrPool == NULL)
    {
        return false;
    }

    pWorkThreadPool = new CWorkThreadPool(pDnseedCfg, pNetWorkService, pBbAddrPool);
    if (pWorkThreadPool == NULL)
    {
        return false;
    }

    pDbStorage = new CDbStorage(&pDnseedCfg->tDbCfg, pBbAddrPool);
    if (pDbStorage == NULL)
    {
        return false;
    }

    pBbAddrPool->SetDbStorage(pDbStorage);
    pBbAddrPool->SetTrustAddr(pCfg->setTrustAddr);

    pDbStorage->SetStatParam(pCfg->fShowDbStatData, pCfg->nShowDbStatTime);

    tmPrevStatTime = time(NULL);

    return true;
}

void CDispatcher::Deinitialize()
{
    if (pNetWorkService)
    {
        delete pNetWorkService;
        pNetWorkService = NULL;
    }

    if (pWorkThreadPool)
    {
        delete pWorkThreadPool;
        pWorkThreadPool = NULL;
    }

    if (pBbAddrPool)
    {
        delete pBbAddrPool;
        pBbAddrPool = NULL;
    }

    if (pDbStorage)
    {
        delete pDbStorage;
        pDbStorage = NULL;
    }
}

bool CDispatcher::StartService()
{
    if (pNetWorkService == NULL || pWorkThreadPool == NULL || pDbStorage == NULL)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Error object.");
        return false;
    }

    if (pNetWorkService->AddTcpListen(pDnseedCfg->tNetCfg.tListenEpIPV4) == 0)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Add ipv4 tcp listen fail.");
        return false;
    }
    if (pNetWorkService->AddTcpListen(pDnseedCfg->tNetCfg.tListenEpIPV6) == 0)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Add ipv6 tcp listen fail.");
        return false;
    }

    if (!pNetWorkService->StartService())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Net work service start fail.");
        StopService();
        return false;
    }

    if (!pWorkThreadPool->StartAll())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Work thread pool start fail.");
        StopService();
        return false;
    }

    if (!pDbStorage->Start())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Db storage start fail.");
        StopService();
        return false;
    }

    return true;
}

void CDispatcher::StopService()
{
    if (pWorkThreadPool)
    {
        pWorkThreadPool->StopAll();
    }
    if (pNetWorkService)
    {
        pNetWorkService->StopService();
    }
    if (pDbStorage)
    {
        pDbStorage->Stop();
    }
}

void CDispatcher::Timer()
{
    time_t tmCurTime = time(NULL);
    if (tmCurTime < tmPrevStatTime || tmCurTime - tmPrevStatTime >= pDnseedCfg->nShowRunStatTime)
    {
        tmPrevStatTime = tmCurTime;

        if (pWorkThreadPool)
        {
            CRunStatData tStatData;
            pWorkThreadPool->StatRunData(tStatData);

            if (pDnseedCfg->fShowRunStatData)
            {
                char sTempBuf[512] = { 0 };
                sprintf(sTempBuf, "Session:%d {In:%d Out:%d}, Total: {In:%ld-%ld (S:%ld-%ld F:%ld-%ld), Out:%ld-%ld (S:%ld-%ld F:%ld-%ld)}",
                        tStatData.nTcpConnCount, tStatData.nInBoundCount, tStatData.nOutBoundCount,
                        tStatData.nTotalInCount, (tStatData.nTotalInCount - tPrevStatData.nTotalInCount) / pDnseedCfg->nShowRunStatTime,
                        tStatData.nTotalInWorkSuccessCount, (tStatData.nTotalInWorkSuccessCount - tPrevStatData.nTotalInWorkSuccessCount) / pDnseedCfg->nShowRunStatTime,
                        tStatData.nTotalInWorkFailCount, (tStatData.nTotalInWorkFailCount - tPrevStatData.nTotalInWorkFailCount) / pDnseedCfg->nShowRunStatTime,
                        tStatData.nTotalOutCount, (tStatData.nTotalOutCount - tPrevStatData.nTotalOutCount) / pDnseedCfg->nShowRunStatTime,
                        tStatData.nTotalOutWorkSuccessCount, (tStatData.nTotalOutWorkSuccessCount - tPrevStatData.nTotalOutWorkSuccessCount) / pDnseedCfg->nShowRunStatTime,
                        tStatData.nTotalOutFailCount + tStatData.nTotalOutWorkFailCount,
                        ((tStatData.nTotalOutFailCount + tStatData.nTotalOutWorkFailCount) - (tPrevStatData.nTotalOutFailCount + tPrevStatData.nTotalOutWorkFailCount)) / pDnseedCfg->nShowRunStatTime);
                blockhead::StdLog("STAT", sTempBuf);
            }

            tPrevStatData = tStatData;
        }
    }
}

} // namespace dnseed
