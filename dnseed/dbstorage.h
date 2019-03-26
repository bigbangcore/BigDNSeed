// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_DBSTORAGE_H
#define __DNSEED_DBSTORAGE_H

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "blockhead/type.h"
#include "dbc/dbcacc.h"
#include "nbase/mthbase.h"

namespace dnseed
{

using namespace std;
using namespace dbc;
using namespace nbase;

#define DDN_D_STAT_TIME 1

typedef enum _DDN_E_MSG_TYPE
{
    DDN_E_MSG_TYPE_FETCH,
    DDN_E_MSG_TYPE_INSERT,
    DDN_E_MSG_TYPE_DELETE,
    DDN_E_MSG_TYPE_UPDATE

} DDN_E_MSG_TYPE, *P_DDN_E_MSG_TYPE;


class CDNSeedNode
{
public:
    CDNSeedNode(DDN_E_MSG_TYPE e) : eMsgType(e) {}
    CDNSeedNode(DDN_E_MSG_TYPE e, string sIpIn, uint16 nPortIn, uint64 nServiceIn, int iScoreIn) : 
        eMsgType(e), strIp(sIpIn),nPort(nPortIn),nService(nServiceIn),iScore(iScoreIn) {}
    CDNSeedNode(CDNSeedNode& tNode) : 
        eMsgType(tNode.eMsgType), strIp(tNode.strIp),nPort(tNode.nPort),nService(tNode.nService),iScore(tNode.iScore) {}
    ~CDNSeedNode() {}

    DDN_E_MSG_TYPE eMsgType;

    string strIp;
    uint16 nPort;
    uint64 nService;
    int iScore;
};


class CBbAddrPool;

class CDbStorage
{
public:
    CDbStorage();
    CDbStorage(CDbcConfig *pDbCfg, CBbAddrPool* pPool);
    ~CDbStorage();

    void SetDbConfig(CDbcConfig *pDbCfg);
    void SetBbAddrPool(CBbAddrPool* pPool);

    bool Start();
    void Stop();

    bool PurgeData();
    bool PostDbMessage(CDNSeedNode* pMsg);
    bool ReqFetchAddr();
    uint32 GetMsgQueueSize();

    void SetStatParam(bool fShowStatIn, uint32 nStatTimeIn);
    
private:
    void Work();
    void DoTimer();
    void DoMessage(CDNSeedNode* pNode);

    bool CreateTables();

    void HandleFetchAddr();
    void HandleInsertNode(CDNSeedNode* pNode);
    void HandleUpdateNode(CDNSeedNode* pNode);
    void HandleDeleteNode(CDNSeedNode* pNode);

    bool QueryAddrIfExist(CDNSeedNode* pNode);

private:
    bool fRunFlag;
    boost::thread *pThreadDbAcc;

    CMthQueue<CDNSeedNode*> tDbMsgQueue;
    CDbcDbConnect *pDbConn;
    CBbAddrPool* pAddrPool;

    uint32 nCfgStatTimeLen;
    bool fCfgShowStat;

    time_t tmPrevTimerTime;
    time_t tmPrevStatTime;

    uint64 nInsertCount;
    uint64 nDeleteCount;
    uint64 nUpdateCount;
    uint64 nPrevInsertCount;
    uint64 nPrevDeleteCount;
    uint64 nPrevUpdateCount;
};

} //namespace dnseed

#endif //__DNSEED_DBSTORAGE_H
