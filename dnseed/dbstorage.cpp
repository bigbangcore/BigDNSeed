// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbstorage.h"
#include "addrpool.h"

using namespace std;

namespace dnseed
{


CDbStorage::CDbStorage() : 
    pAddrPool(NULL),pThreadDbAcc(NULL),fRunFlag(false),pDbConn(NULL)
{
    nCfgStatTimeLen = DDN_D_STAT_TIME;
    fCfgShowStat = true;
    nInsertCount = 0;
    nDeleteCount = 0;
    nUpdateCount = 0;
    nPrevInsertCount = 0;
    nPrevDeleteCount = 0;
    nPrevUpdateCount = 0;
}

CDbStorage::CDbStorage(CDbcConfig *pDbCfg, CBbAddrPool* pPool) : 
    pAddrPool(pPool),pThreadDbAcc(NULL),fRunFlag(false),pDbConn(NULL)
{
    nCfgStatTimeLen = DDN_D_STAT_TIME;
    fCfgShowStat = true;
    nInsertCount = 0;
    nDeleteCount = 0;
    nUpdateCount = 0;
    nPrevInsertCount = 0;
    nPrevDeleteCount = 0;
    nPrevUpdateCount = 0;
    pDbConn = CDbcDbConnect::DbcCreateDbConnObj(*pDbCfg);
}

CDbStorage::~CDbStorage()
{
    Stop();

    if (pDbConn)
    {
        pDbConn->DisconnectDb();
        pDbConn->Release();
        pDbConn = NULL;
    }
}


void CDbStorage::SetDbConfig(CDbcConfig *pDbCfg)
{
    pDbConn = CDbcDbConnect::DbcCreateDbConnObj(*pDbCfg);
}

void CDbStorage::SetBbAddrPool(CBbAddrPool* pPool)
{
    pAddrPool = pPool;
}

bool CDbStorage::Start()
{
    fRunFlag = true;
    pThreadDbAcc = new boost::thread(boost::bind(&CDbStorage::Work, this));
    if (pThreadDbAcc == NULL)
    {
        return false;
    }
    return true;
}

void CDbStorage::Stop()
{
    fRunFlag = false;

    if (pThreadDbAcc)
    {
        pThreadDbAcc->join();

        delete pThreadDbAcc;
        pThreadDbAcc = NULL;
    }
}

bool CDbStorage::PurgeData()
{
    if (!pDbConn->ExecuteStaticSql("DROP TABLE dnseednode"))
    {
        cerr << "Purge data fail.\n";
        return false;
    }
    return true;
}

bool CDbStorage::PostDbMessage(CDNSeedNode* pMsg)
{
    return tDbMsgQueue.SetData(pMsg);
}

bool CDbStorage::ReqFetchAddr()
{
    CDNSeedNode *pNode = new CDNSeedNode(DDN_E_MSG_TYPE_FETCH);
    return tDbMsgQueue.SetData(pNode);
}

uint32 CDbStorage::GetMsgQueueSize()
{
    return tDbMsgQueue.GetCount();
}

void CDbStorage::SetStatParam(bool fShowStatIn, uint32 nStatTimeIn)
{
    fCfgShowStat = fShowStatIn;
    nCfgStatTimeLen = nStatTimeIn;
}


//----------------------------------------------------------------------------
void CDbStorage::Work()
{
    CreateTables();
    HandleFetchAddr();

    tmPrevTimerTime = time(NULL);
    tmPrevStatTime = tmPrevTimerTime;

    while (fRunFlag)
    {
        DoTimer();
        CDNSeedNode *pNode = NULL;
        if (!tDbMsgQueue.GetData(pNode, 100) || pNode == NULL)
        {
            continue;
        }
        DoMessage(pNode);
        delete pNode;
    }
}

void CDbStorage::DoTimer()
{
    time_t tmCurTime = time(NULL);

    if (tmCurTime - tmPrevTimerTime >= 1 || tmCurTime < tmPrevTimerTime)
    {
        tmPrevTimerTime = tmCurTime;

        pDbConn->Timer();

        int iDbCommitCount = tDbMsgQueue.GetCount() / 10;
        if (iDbCommitCount >= 10)
        {
            pDbConn->SetCommitCount(iDbCommitCount);
        }
        else if (iDbCommitCount <= 1)
        {
            pDbConn->SetCommitCount(0);
        }
    }

    if (tmCurTime - tmPrevStatTime >= nCfgStatTimeLen || tmCurTime < tmPrevStatTime)
    {
        tmPrevStatTime = tmCurTime;

        if (fCfgShowStat)
        {
            char sBuf[512] = {0};
            sprintf(sBuf, "db queue: %d, Insert: %ld-%ld, Delete: %ld-%ld, Update: %ld-%ld.", 
                tDbMsgQueue.GetCount(), 
                nInsertCount, (nInsertCount - nPrevInsertCount) / nCfgStatTimeLen, 
                nDeleteCount, (nDeleteCount - nPrevDeleteCount) / nCfgStatTimeLen, 
                nUpdateCount, (nUpdateCount - nPrevUpdateCount) / nCfgStatTimeLen);
            blockhead::StdLog("STAT", sBuf);
        }

        nPrevInsertCount = nInsertCount;
        nPrevDeleteCount = nDeleteCount;
        nPrevUpdateCount = nUpdateCount;
    }
}

void CDbStorage::DoMessage(CDNSeedNode* pNode)
{
    switch (pNode->eMsgType)
    {
    case DDN_E_MSG_TYPE_FETCH:
        HandleFetchAddr();
        break;
    case DDN_E_MSG_TYPE_INSERT:
        HandleInsertNode(pNode);
        nInsertCount++;
        break;
    case DDN_E_MSG_TYPE_DELETE:
        HandleDeleteNode(pNode);
        nDeleteCount++;
        break;
    case DDN_E_MSG_TYPE_UPDATE:
        HandleUpdateNode(pNode);
        nUpdateCount++;
        break;
    }
}


bool CDbStorage::CreateTables()
{
    if (!pDbConn->ExecuteStaticSql("CREATE TABLE IF NOT EXISTS dnseednode("
                        "id INT NOT NULL AUTO_INCREMENT,"
                        "address varchar(64) NOT NULL,"
                        "port INT NOT NULL,"
                        "service BIGINT NOT NULL,"
                        "score INT NOT NULL,"
                        "PRIMARY KEY (id),"
                        "KEY i_address_port (address,port))"))
    {
        cerr << "create tables fail.\n";
        return false;
    }
    return true;
}

void CDbStorage::HandleFetchAddr()
{
    std::string strSql = "SELECT address,port,service,score FROM dnseednode";
    CDbcSelect* pSelect = pDbConn->Query(strSql);
    if (pSelect == NULL)
    {
        return;
    }

    while (pSelect->MoveNext())
    {
        string strIp;
        int iPort;
        uint64 nService;
        int iScore;

        if (!pSelect->GetField(0, strIp))
        {
            break;
        }
        if (!pSelect->GetField(1, iPort))
        {
            break;
        }
        if (!pSelect->GetField(2, nService))
        {
            break;
        }
        if (!pSelect->GetField(3, iScore))
        {
            break;
        }

        CMthNetEndpoint ep;
        if (!ep.SetAddrPort(strIp, uint16(iPort)))
        {
            continue;
        }
        pAddrPool->AddAddrFromDb(ep, nService, iScore);
    }

    pSelect->Release();
}

void CDbStorage::HandleInsertNode(CDNSeedNode* pNode)
{
    if (!QueryAddrIfExist(pNode))
    {
        std::ostringstream oss;
        oss << "INSERT INTO dnseednode(address,port,service,score) VALUES("
            << "\'" << pDbConn->ToEscString(pNode->strIp) << "\',"
            << pNode->nPort << ","
            << pNode->nService << ","
            << pNode->iScore << ")";
        std::string strSql = oss.str();
        pDbConn->ExecuteStaticSql(strSql);
    }
}

void CDbStorage::HandleUpdateNode(CDNSeedNode* pNode)
{
    std::ostringstream oss;
    oss << "UPDATE dnseednode SET service="<<pNode->nService <<",score=" << pNode->iScore << " WHERE address=\'" 
        << pDbConn->ToEscString(pNode->strIp) << "\' AND port=" << pNode->nPort;
    std::string strSql = oss.str();
    pDbConn->ExecuteStaticSql(strSql);
}

void CDbStorage::HandleDeleteNode(CDNSeedNode* pNode)
{
    std::ostringstream oss;
    oss << "DELETE FROM dnseednode WHERE address=\'" 
        << pDbConn->ToEscString(pNode->strIp) << "\' AND port=" << pNode->nPort;
    std::string strSql = oss.str();
    pDbConn->ExecuteStaticSql(strSql);
}

bool CDbStorage::QueryAddrIfExist(CDNSeedNode* pNode)
{
    std::ostringstream oss;
    oss << "SELECT count(*) FROM dnseednode WHERE address=\'" 
        << pDbConn->ToEscString(pNode->strIp) << "\' AND port=" << pNode->nPort;
    std::string strSql = oss.str();

    CDbcSelect* pSelect = pDbConn->Query(strSql);
    if (pSelect == NULL || !pSelect->MoveNext())
    {
        pSelect->Release();   
        return false;
    }

    uint32 nCount = 0;
    if (!pSelect->GetField(0, nCount) || nCount == 0)
    {
        pSelect->Release();   
        return false;
    }
    pSelect->Release();
    
    return true;
}

}  // namespace dnseed

