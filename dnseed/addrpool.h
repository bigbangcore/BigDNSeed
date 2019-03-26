// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_ADDRPOOL_H
#define __DNSEED_ADDRPOOL_H

#include "network/networkbase.h"
#include "blockhead/nettime.h"
#include "nbase/mthbase.h"
#include "netproto.h"
#include "config.h"


namespace dnseed
{

using namespace std;
using namespace nbase;
using namespace network;
using namespace blockhead;



class CAddrTestParam
{
public:
    CAddrTestParam()
    {
        Init();
    }
    ~CAddrTestParam() {}

    void Init()
    {
        nConnectCount = 0;
        nPrevConnectTime = GetTime();
        nNextConnIntervalTime = NMS_ATP_TEST_CONN_INTERVAL_INIT_TIME;
    }

    CAddrTestParam& operator=(CAddrTestParam& a)
    {
        nConnectCount = a.nConnectCount;
        nPrevConnectTime = a.nPrevConnectTime;
        nNextConnIntervalTime = a.nNextConnIntervalTime;
        return *this;
    }

public:
    uint32 nConnectCount;
    int64 nPrevConnectTime;
    uint32 nNextConnIntervalTime;
};

class CBbAddr
{
    friend class CBbAddrPool;
public:
    CBbAddr();
    ~CBbAddr() {}

    bool SetBbAddr(string& strIp, uint16 nPort, uint64 nServiceIn, int iScoreIn);
    bool SetBbAddr(tcp::endpoint& ep, uint64 nServiceIn, int iScoreIn);
    bool GetAddress(CAddress& tAddr);
    bool GetAddress(tcp::endpoint& ep, uint64& nServiceOut, int& iScoreOut);

    CMthNetEndpoint& GetEp() {return tNetEp;}
    int GetScore() const {return iScore;}
    uint64 GetService() const {return nService;}

    void DoScore(int iDoValue);
    void DoScoreByHeight(int nRefHeight);

    CBbAddr& operator=(CBbAddr& ad)
    {
        tNetEp = ad.tNetEp;
        iScore = ad.iScore;
        fConfidentAddr = ad.fConfidentAddr;
        nStartingHeight = ad.nStartingHeight;
        tTestParam = ad.tTestParam;
        return *this;
    }

public:
    CMthNetEndpoint tNetEp;
    uint64 nService;
    int iScore;
    bool fConfidentAddr;
    int nStartingHeight;

    CAddrTestParam tTestParam;
};

class CDbStorage;

class CBbAddrPool
{
public:
    CBbAddrPool(CDnseedConfig *pCfg);
    ~CBbAddrPool();

    void SetDbStorage(CDbStorage* pDbs);
    bool SetTrustAddr(set<string>& setTrustAddrIn);

    bool AddConfidentAddr(string& sAddr);
    bool AddAddrFromDb(CMthNetEndpoint &ep, uint64 nService, int iScore);
    bool AddRecvAddr(tcp::endpoint& ep, uint64 nServiceIn);
    void DelAddr(CMthNetEndpoint &ep);
    void DelAddr(CBbAddr &addr);

    bool QueryAddr(CMthNetEndpoint &ep, CBbAddr& addr);
    bool DoScore(CMthNetEndpoint &ep, int iDoValue);
    bool DoScore(CBbAddr& addr, int iDoValue);
    bool UpdateHeight(CMthNetEndpoint &ep, int iHeight);

    void GetGoodAddressList(vector<CAddress>& vAddrList);
    bool GetCallConnectAddrList(vector<CBbAddr>& vBbAddr, uint32 nGetAddrCount, bool fStressTest);

    int64 GetNetTime();
    bool UpdateNetTime(const boost::asio::ip::address& address,int64 nTimeDelta);

    void SetConfidentHeight(uint32 nHeight);
    uint32 GetConfidentHeight();

protected:
    void ReleaseAddrPool();
    CBbAddr* GetAddrNoLock(const string& sAddrPort);

private:
    CDnseedConfig *pDnseedCfg;

    map<string, CBbAddr*> mapAddrPool;
    set<string> setConfidentAddrPool;
    boost::shared_mutex lockAddrPool;

    uint32 nConfidentHeight;
    boost::shared_mutex lockConfidentAddr;

    CBlockheadNetTime tmNet;
    boost::shared_mutex lockNetTime;

    CDbStorage* pDbStorage;

    string sPrevTestAddr;
};

}  // namespace dnseed

#endif //__DNSEED_ADDRPOOL_H
