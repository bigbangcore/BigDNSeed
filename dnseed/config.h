// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_CONFIG_H
#define __DNSEED_CONFIG_H

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "blockhead/type.h"
#include "crypto/uint256.h"
#include "dbc/dbcacc.h"
#include "network/networkbase.h"

namespace dnseed
{

using namespace std;
using namespace dbc;
using namespace network;

//#define NMS_CFG_LISTEN_PORT 9906
#define NMS_CFG_LISTEN_PORT 8806 //MKF
#define NMS_ATP_MAX_ADDR_SCORE 100
#define NMS_ATP_MIN_ADDR_SCORE -200
#define NMS_ATP_GOOD_ADDR_SCORE 10
#define NMS_ATP_TEST_CONN_INTERVAL_INIT_TIME 5
#define NMS_ATP_TEST_CONN_INTERVAL_START_TIME 60
#define NMS_ATP_TEST_CONN_INTERVAL_MAX_TIME 7200
#define NMS_ATP_HEIGHT_DIFF_RANGE 20
#define NMS_ATP_GET_GOOD_ADDR_COUNT 8
#define NMS_ATP_TEST_ADDR_COUNT 30

class CNetConfig
{
public:
    CNetConfig() {}
    ~CNetConfig() {}

public:
    CMthNetEndpoint tListenEpIPV4;
    CMthNetEndpoint tListenEpIPV6;
};

class CDnseedConfig
{
public:
    CDnseedConfig();
    ~CDnseedConfig();

    bool ReadConfig(int argc, char* argv[],
                    const boost::filesystem::path& pathDefault, const std::string& strConfile);
    void ListConfig();
    void Help();

private:
    bool TokenizeConfile(const char* pzConfile, vector<string>& tokens);
    void AddToken(string& sIn, vector<string>& tokens);
    static std::pair<std::string, std::string> ExtraParser(const std::string& s);

public:
    CDbcConfig tDbCfg;
    CNetConfig tNetCfg;

    bool fStop;

    bool fTestNet;
    bool fVersion;
    bool fPurge;

    bool fDebug;
    bool fHelp;
    bool fDaemon;
    unsigned int nMagicNum;
    int nLogFileSize;
    int nLogHistorySize;

    string sWorkDir;
    bool fAllowAllAddr;
    uint32 nWorkThreadCount;

    bool fStressBackTest;
    uint32 nGetGoodAddrCount;
    uint32 nBackTestAddrCount;
    int nGoodAddrScore;

    bool fShowRunStatData;
    uint32 nShowRunStatTime;

    bool fShowDbStatData;
    uint32 nShowDbStatTime;

    uint256 hashGenesisBlock;

    //-----------------------------------------------------------
    boost::filesystem::path pathRoot;
    boost::filesystem::path pathData;
    boost::filesystem::path pathConfile;

    boost::program_options::options_description defaultDesc;
    set<string> setTrustAddr;

    string strGenesisBlockHash;
};

class CRunStatData
{
public:
    CRunStatData()
    {
        nTcpConnCount = 0;
        nInBoundCount = 0;
        nOutBoundCount = 0;

        nTotalInCount = 0;
        nTotalInWorkSuccessCount = 0;
        nTotalInWorkFailCount = 0;

        nTotalOutCount = 0;
        nTotalOutSuccessCount = 0;
        nTotalOutFailCount = 0;
        nTotalOutWorkSuccessCount = 0;
        nTotalOutWorkFailCount = 0;
    }
    ~CRunStatData() {}

    CRunStatData& operator=(CRunStatData& t)
    {
        nTcpConnCount = t.nTcpConnCount;
        nInBoundCount = t.nInBoundCount;
        nOutBoundCount = t.nOutBoundCount;

        nTotalInCount = t.nTotalInCount;
        nTotalInWorkSuccessCount = t.nTotalInWorkSuccessCount;
        nTotalInWorkFailCount = t.nTotalInWorkFailCount;

        nTotalOutCount = t.nTotalOutCount;
        nTotalOutSuccessCount = t.nTotalOutSuccessCount;
        nTotalOutFailCount = t.nTotalOutFailCount;
        nTotalOutWorkSuccessCount = t.nTotalOutWorkSuccessCount;
        nTotalOutWorkFailCount = t.nTotalOutWorkFailCount;
        return *this;
    }

    CRunStatData& operator+=(CRunStatData& t)
    {
        nTcpConnCount += t.nTcpConnCount;
        nInBoundCount += t.nInBoundCount;
        nOutBoundCount += t.nOutBoundCount;

        nTotalInCount += t.nTotalInCount;
        nTotalInWorkSuccessCount += t.nTotalInWorkSuccessCount;
        nTotalInWorkFailCount += t.nTotalInWorkFailCount;

        nTotalOutCount += t.nTotalOutCount;
        nTotalOutSuccessCount += t.nTotalOutSuccessCount;
        nTotalOutFailCount += t.nTotalOutFailCount;
        nTotalOutWorkSuccessCount += t.nTotalOutWorkSuccessCount;
        nTotalOutWorkFailCount += t.nTotalOutWorkFailCount;
        return *this;
    }

public:
    uint32 nTcpConnCount;
    uint32 nInBoundCount;
    uint32 nOutBoundCount;

    uint64 nTotalInCount;
    uint64 nTotalInWorkSuccessCount;
    uint64 nTotalInWorkFailCount;

    uint64 nTotalOutCount;
    uint64 nTotalOutSuccessCount;
    uint64 nTotalOutFailCount;
    uint64 nTotalOutWorkSuccessCount;
    uint64 nTotalOutWorkFailCount;
};

} //namespace dnseed

#endif //__DNSEED_CONFIG_H
