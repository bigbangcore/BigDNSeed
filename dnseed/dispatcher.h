// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_DISPATCHER_H
#define __DNSEED_DISPATCHER_H

#include "addrpool.h"
#include "config.h"
#include "dbstorage.h"
#include "netmsgwork.h"
#include "network/networkservice.h"

namespace dnseed
{

using namespace std;
using namespace network;

class CDispatcher
{
public:
    CDispatcher();
    ~CDispatcher();

    bool Initialize(CDnseedConfig* pCfg);
    void Deinitialize();

    bool StartService();
    void StopService();

    void Timer();

private:
    CDnseedConfig* pDnseedCfg;

    CNetWorkService* pNetWorkService;
    CWorkThreadPool* pWorkThreadPool;
    CDbStorage* pDbStorage;
    CBbAddrPool* pBbAddrPool;

    CRunStatData tPrevStatData;
    time_t tmPrevStatTime;
};

} //namespace dnseed

#endif //__DNSEED_DISPATCHER_H
