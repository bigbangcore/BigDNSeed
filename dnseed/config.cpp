// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "config.h"

#include <boost/algorithm/string/trim.hpp>

#include "blockhead/util.h"
#include "netproto.h"

using namespace std;

namespace dnseed
{

namespace po = boost::program_options;
namespace fs = boost::filesystem;

CDnseedConfig::CDnseedConfig()
{
    fStop = false;

    fTestNet = false;
    fVersion = false;
    fPurge = false;

    fDebug = false;
    fHelp = false;
    fDaemon = false;
    nWorkThreadCount = 0;
    fAllowAllAddr = false;

    fStressBackTest = false;
    nGetGoodAddrCount = NMS_ATP_GET_GOOD_ADDR_COUNT;
    nBackTestAddrCount = NMS_ATP_TEST_ADDR_COUNT;

    fShowRunStatData = true;
    nShowRunStatTime = 1;

    fShowDbStatData = false;
    nShowDbStatTime = 10;

    tDbCfg.iDbType = DBC_DBTYPE_MYSQL;
    tDbCfg.sDbIp = "localhost";
    tDbCfg.usDbPort = 3306;
    tDbCfg.sDbName = "bigdnseedmkf";
    tDbCfg.sDbUser = "bigdnseed";
    tDbCfg.sDbPwd = "bigdnseed";

    tNetCfg.tListenEpIPV4.SetAddrPort("0.0.0.0", NMS_CFG_LISTEN_PORT);
    tNetCfg.tListenEpIPV6.SetAddrPort("::", NMS_CFG_LISTEN_PORT);
}

CDnseedConfig::~CDnseedConfig()
{
}

bool CDnseedConfig::ReadConfig(int argc, char* argv[],
                               const boost::filesystem::path& pathDefault, const std::string& strConfile)
{
    pathRoot = pathDefault;
    pathData = pathRoot;

    po::variables_map vm;
    string strGenesisBlockHash;
    string strDNSeedListenAddrV4;
    string strDNSeedListenAddrV6;
    unsigned short usDNSeedListenPort;

    const int defaultCmdStyle = po::command_line_style::allow_long
                                | po::command_line_style::long_allow_adjacent
                                | po::command_line_style::allow_long_disguise;

    defaultDesc.add_options()
        //help
        ("help", po::value<bool>(&fHelp)->default_value(false), "Get more information")
        //stop
        ("stop", po::value<bool>(&fStop)->default_value(false), "Stop service")
        //testnet
        ("testnet", po::value<bool>(&fTestNet)->default_value(false), "Use the test network")
        //version
        ("version", po::value<bool>(&fVersion)->default_value(false), "Get bigdnseed version")
        //purge
        ("purge", po::value<bool>(&fPurge)->default_value(false), "Purge database and blockfile")
        //debug
        ("debug", po::value<bool>(&fDebug)->default_value(false), "Run in debug mode")
        //daemon
        ("daemon", po::value<bool>(&fDaemon)->default_value(false), "Run server in background")
        //logfilesize
        ("logfilesize", po::value<int>(&nLogFileSize)->default_value(100), "Single log file size(M) (default: 100M)")
        //loghistorysize
        ("loghistorysize", po::value<int>(&nLogHistorySize)->default_value(2048), "Log history size(M) (default: 2048M)")
        //workdir
        ("workdir", po::value<string>(&sWorkDir)->default_value(pathDefault.c_str()), "Work dir")
        //workthreadcount
        ("workthreadcount", po::value<unsigned int>(&nWorkThreadCount)->default_value(0), "Work thread number(0 is the number of CPUs)")
        //genesisblock
        ("genesisblock", po::value<string>(&strGenesisBlockHash)->default_value(""), "Genesis block hash")
        //allowalladdr
        ("allowalladdr", po::value<bool>(&fAllowAllAddr)->default_value(false), "Allow all address")
        //dbhost
        ("dbhost", po::value<string>(&tDbCfg.sDbIp)->default_value("localhost"), "Set mysql host (default: localhost)")
        //dbport
        ("dbport", po::value<unsigned short>(&tDbCfg.usDbPort)->default_value(3306), "Set mysql port (default: 3306)")
        //dbname
        ("dbname", po::value<string>(&tDbCfg.sDbName)->default_value("bigdnseedmkf"), "Set mysql database name (default: bigdnseedmkf)")
        //dbuser
        ("dbuser", po::value<string>(&tDbCfg.sDbUser)->default_value("bigdnseed"), "Set mysql user's name (default: bigdnseed)")
        //dbpass
        ("dbpass", po::value<string>(&tDbCfg.sDbPwd)->default_value("bigdnseed"), "Set mysql user's password (default: bigdnseed)")
        //listenaddrv4
        ("listenaddrv4", po::value<string>(&strDNSeedListenAddrV4)->default_value("0.0.0.0"), "Listen for connections on <ipv4>")
        //listenaddrv6
        ("listenaddrv6", po::value<string>(&strDNSeedListenAddrV6)->default_value("::"), "Listen for connections on <ipv6>")
        //listenport
        ("listenport", po::value<unsigned short>(&usDNSeedListenPort)->default_value(NMS_CFG_LISTEN_PORT), "Listen for connections on <port>")
        //trustaddress
        ("trustaddress", po::value<std::vector<std::string>>()->multitoken(), "Trust address list")
        //stressbacktest
        ("stressbacktest", po::value<bool>(&fStressBackTest)->default_value(false), "Whether to stress test access nodes")
        //goodaddrscore
        ("goodaddrscore", po::value<int>(&nGoodAddrScore)->default_value(NMS_ATP_GOOD_ADDR_SCORE), "Good address score")
        //getgoodaddrcount
        ("getgoodaddrcount", po::value<unsigned int>(&nGetGoodAddrCount)->default_value(NMS_ATP_GET_GOOD_ADDR_COUNT), "Returns the number of available nodes")
        //backtestaddrcount
        ("backtestaddrcount", po::value<unsigned int>(&nBackTestAddrCount)->default_value(NMS_ATP_TEST_ADDR_COUNT), "Single test node number")
        //showrunstatdata
        ("showrunstatdata", po::value<bool>(&fShowRunStatData)->default_value(true), "Do you want to display running statistics")
        //showrunstattime
        ("showrunstattime", po::value<unsigned int>(&nShowRunStatTime)->default_value(1), "Interval time for displaying running")
        //showdbstatdata
        ("showdbstatdata", po::value<bool>(&fShowDbStatData)->default_value(false), "Do you want to display database access statistics")
        //showdbstattime
        ("showdbstattime", po::value<unsigned int>(&nShowDbStatTime)->default_value(10), "Interval time for displaying database access statistics");

    //----------------------------------------------------------------------------------------------------------
    po::store(po::command_line_parser(argc, argv).options(defaultDesc).style(defaultCmdStyle).extra_parser(CDnseedConfig::ExtraParser).run(), vm);
    po::notify(vm);

    if (vm.count("trustaddress"))
    {
        for (auto& str : vm["trustaddress"].as<std::vector<std::string>>())
        {
            if (setTrustAddr.find(str) == setTrustAddr.end())
            {
                setTrustAddr.insert(str);
            }
        }
    }

    pathRoot = sWorkDir;
    pathData = pathRoot;

    //----------------------------------------------------------------------------------------------------------
    pathConfile = strConfile;
    if (!pathConfile.is_complete())
    {
        pathConfile = pathRoot / pathConfile;
    }

    vector<string> confToken;
    if (TokenizeConfile(pathConfile.string().c_str(), confToken))
    {
        po::store(po::command_line_parser(confToken).options(defaultDesc).style(defaultCmdStyle).extra_parser(CDnseedConfig::ExtraParser).run(), vm);
        po::notify(vm);

        if (vm.count("trustaddress"))
        {
            for (auto& str : vm["trustaddress"].as<std::vector<std::string>>())
            {
                if (setTrustAddr.find(str) == setTrustAddr.end())
                {
                    setTrustAddr.insert(str);
                }
            }
        }
    }

    //----------------------------------------------------------------------------------------------------------
    if (strGenesisBlockHash.empty())
    {
        cout << "genesisblock is empty!" << endl;
        return false;
    }
    if (hashGenesisBlock.SetHex(strGenesisBlockHash) != strGenesisBlockHash.size())
    {
        cout << "genesisblock is error!" << endl;
        return false;
    }

    tNetCfg.tListenEpIPV4.SetAddrPort(strDNSeedListenAddrV4, usDNSeedListenPort);
    tNetCfg.tListenEpIPV6.SetAddrPort(strDNSeedListenAddrV6, usDNSeedListenPort);
    nMagicNum = (fTestNet ? TESTNET_MAGICNUM : MAINNET_MAGICNUM);
    if (fDebug)
    {
        blockhead::STD_DEBUG = true;
    }

    if (nGetGoodAddrCount == 0)
    {
        nGetGoodAddrCount = NMS_ATP_GET_GOOD_ADDR_COUNT;
    }
    else if (nGetGoodAddrCount > 512)
    {
        nGetGoodAddrCount = 512;
    }

    if (nBackTestAddrCount == 0)
    {
        nBackTestAddrCount = NMS_ATP_TEST_ADDR_COUNT;
    }
    else if (nBackTestAddrCount > 2000)
    {
        nBackTestAddrCount = 2000;
    }

    if (nShowRunStatTime == 0)
    {
        nShowRunStatTime = 1;
    }
    else if (nShowRunStatTime > 3600)
    {
        nShowRunStatTime = 3600;
    }

    if (nShowDbStatTime == 0)
    {
        nShowDbStatTime = 10;
    }
    else if (nShowDbStatTime > 3600)
    {
        nShowDbStatTime = 3600;
    }

    return true;
}

void CDnseedConfig::ListConfig()
{
    cout << "help: " << (fHelp ? "true" : "false") << endl;
    cout << "testnet: " << (fTestNet ? "true" : "false") << endl;
    cout << "version: " << (fVersion ? "true" : "false") << endl;
    cout << "purge: " << (fPurge ? "true" : "false") << endl;
    cout << "debug: " << (fDebug ? "true" : "false") << endl;
    cout << "daemon: " << (fDaemon ? "true" : "false") << endl;
    cout << "logfilesize: " << nLogFileSize << endl;
    cout << "loghistorysize: " << nLogHistorySize << endl;
    cout << "workdir: " << sWorkDir << endl;
    cout << "workthreadcount: " << nWorkThreadCount << endl;
    cout << "allowalladdr: " << (fAllowAllAddr ? "true" : "false") << endl;
    cout << "genesisblock: " << hashGenesisBlock.GetHex() << endl;
    cout << "dbhost: " << tDbCfg.sDbIp << endl;
    cout << "dbport: " << tDbCfg.usDbPort << endl;
    cout << "dbname: " << tDbCfg.sDbName << endl;
    cout << "dbuser: " << tDbCfg.sDbUser << endl;
    cout << "dbpass: " << tDbCfg.sDbPwd << endl;
    cout << "listenaddrv4: " << tNetCfg.tListenEpIPV4.GetIp() << endl;
    cout << "listenportv4: " << tNetCfg.tListenEpIPV4.GetPort() << endl;
    cout << "listenaddrv6: " << tNetCfg.tListenEpIPV6.GetIp() << endl;
    cout << "listenportv6: " << tNetCfg.tListenEpIPV6.GetPort() << endl;
    cout << "trustaddress: " << setTrustAddr.size() << endl;
    for (auto addr : setTrustAddr)
    {
        cout << "addr: " << addr << endl;
    }

    cout << "stressbacktest: " << (fStressBackTest ? "true" : "false") << endl;
    cout << "goodaddrscore: " << nGoodAddrScore << endl;
    cout << "getgoodaddrcount: " << nGetGoodAddrCount << endl;
    cout << "backtestaddrcount: " << nBackTestAddrCount << endl;

    cout << "showrunstatdata: " << (fShowRunStatData ? "true" : "false") << endl;
    cout << "showrunstattime: " << nShowRunStatTime << endl;

    cout << "showdbstatdata: " << (fShowDbStatData ? "true" : "false") << endl;
    cout << "showdbstattime: " << nShowDbStatTime << endl;
}

void CDnseedConfig::Help()
{
    cout << defaultDesc << endl;
}

bool CDnseedConfig::TokenizeConfile(const char* pzConfile, vector<string>& tokens)
{
    ifstream ifs(pzConfile);
    if (!ifs)
    {
        return false;
    }
    string line;
    while (!getline(ifs, line).eof() || !line.empty())
    {
        string s = line.substr(0, line.find('#'));
        boost::trim(s);
        if (!s.empty())
        {
            AddToken(s, tokens);
        }
        line.clear();
    }
    return true;
}

void CDnseedConfig::AddToken(string& sIn, vector<string>& tokens)
{
    size_t pos1, pos2;
    string sFieldName;
    string sTempValue;

    pos1 = sIn.find("=");
    if (pos1 == string::npos)
    {
        tokens.push_back(string("-") + sIn);
        return;
    }
    sFieldName = sIn.substr(0, pos1);
    boost::trim(sFieldName);
    if (sFieldName.empty())
    {
        return;
    }
    sTempValue = sIn.substr(pos1 + 1, -1);
    boost::trim(sTempValue);
    if (sTempValue.empty())
    {
        return;
    }
    pos1 = sTempValue.find(" ");
    if (pos1 == string::npos)
    {
        tokens.push_back(string("-") + sFieldName + string("=") + sTempValue);
    }
    else
    {
        string sTempStr;

        sTempStr = sTempValue.substr(0, pos1);
        boost::trim(sTempStr);
        tokens.push_back(string("-") + sFieldName + string("=") + sTempStr);

        sTempStr = sTempValue.substr(pos1 + 1, -1);
        boost::trim(sTempStr);
        if (sTempStr.empty())
        {
            return;
        }

        pos1 = 0;
        while (1)
        {
            pos2 = sTempStr.find(" ", pos1);
            if (pos2 == string::npos)
            {
                sTempValue = sTempStr.substr(pos1, -1);
                if (!sTempValue.empty())
                {
                    tokens.push_back(sTempValue);
                }
                break;
            }
            sTempValue = sTempStr.substr(pos1, pos2 - pos1);
            boost::trim(sTempValue);
            if (!sTempValue.empty())
            {
                tokens.push_back(sTempValue);
            }
            pos1 = pos2 + 1;
        }
    }
}

pair<string, string> CDnseedConfig::ExtraParser(const string& s)
{
    if (s[0] == '-' && !isdigit(s[1]))
    {
        bool fRev = (s.substr(1, 2) == "no");
        size_t eq = s.find('=');
        if (eq == string::npos)
        {
            if (fRev)
            {
                return make_pair(s.substr(3), string("0"));
            }
            else
            {
                return make_pair(s.substr(1), string("1"));
            }
        }
        else if (fRev)
        {
            int v = atoi(s.substr(eq + 1).c_str());
            return make_pair(s.substr(3, eq - 3), string(v != 0 ? "0" : "1"));
        }
    }
    return make_pair(string(), string());
}

} // namespace dnseed
