// stresstest.cpp

#include "stresstest.h"
#include "dnseed/version.h"
#include <boost/algorithm/string/trim.hpp>

using namespace dnseed;

namespace po = boost::program_options; 
namespace fs = boost::filesystem; 

namespace stresstest
{


//---------------------------------------------------------------------------
CStressTestConfig::CStressTestConfig()
{
    fHelp = false;
    fTestNet = false;
    fDebug = false;
    nMagicNum = MAINNET_MAGICNUM;
    tNetCfg.tListenEpIPV4.SetAddrPort("0.0.0.0", 6817);
    tNetCfg.tListenEpIPV6.SetAddrPort("::", 6817);

    sLocalIpaddr = "127.0.0.1";
    iLocalIpType = CMthNetIp::MNI_IPV4;
    nListenBeginPort = 10000;
    nListenPortCount = 1;
    nPerPortCount = 1;

    sConnectIpaddr = "127.0.0.1";
    nConnectPort = 6819;
    nPerConnectCount = 1;
    nMaxConnectCount = 1;

    fRecvTrustConnect = false;
}

CStressTestConfig::~CStressTestConfig()
{

}

bool CStressTestConfig::ReadConfig(int argc, char *argv[], 
    const boost::filesystem::path& pathDefault, const std::string& strConfile)
{
    pathRoot = pathDefault;
    pathData = pathRoot;

    po::variables_map vm;

    const int defaultCmdStyle = po::command_line_style::allow_long 
                         | po::command_line_style::long_allow_adjacent
                         | po::command_line_style::allow_long_disguise;

    defaultDesc.add_options()
        ("help", po::value<bool>(&fHelp)->default_value(false), "Get more information")
        ("testnet", po::value<bool>(&fTestNet)->default_value(false), "Use the test network")
        ("debug", po::value<bool>(&fDebug)->default_value(false), "Run in debug mode")
        ("workdir", po::value<string>(&sWorkDir)->default_value(pathDefault.c_str()), "work dir")
        ("trustport", po::value<unsigned short>(&nTrustPort)->default_value(0), "begin port")
        ("localip", po::value<string>(&sLocalIpaddr)->default_value("127.0.0.1"), "local ip")
        ("beginport", po::value<unsigned short>(&nListenBeginPort)->default_value(10000), "begin port")
        ("portcount", po::value<unsigned short>(&nListenPortCount)->default_value(1), "port count")
        ("perportcount", po::value<unsigned int>(&nPerPortCount)->default_value(10), "per port count")
        ("connectip", po::value<string>(&sConnectIpaddr)->default_value("127.0.0.1"), "connect ip")
        ("connectport", po::value<unsigned short>(&nConnectPort)->default_value(6819), "connect port")
        ("perconnectcount", po::value<unsigned int>(&nPerConnectCount)->default_value(1), "per connect count")
        ("maxconnectcount", po::value<unsigned int>(&nMaxConnectCount)->default_value(1), "max connect count")
        ;

    //----------------------------------------------------------------------------------------------------------
    po::store(po::command_line_parser(argc, argv).options(defaultDesc).style(defaultCmdStyle)
        .extra_parser(CStressTestConfig::ExtraParser).run(),vm);
    po::notify(vm);

    //printf("pathDefault: %s, sWorkDir: %s.\n", pathDefault.c_str(), sWorkDir.c_str());

    pathRoot = sWorkDir;
    pathData = pathRoot;

    //----------------------------------------------------------------------------------------------------------
    pathConfile = strConfile;
    if (!pathConfile.is_complete())
    {
        pathConfile = pathRoot / pathConfile;
    }

    vector<string> confToken;
    if (TokenizeConfile(pathConfile.string().c_str(),confToken))
    {
        po::store(po::command_line_parser(confToken).options(defaultDesc).style(defaultCmdStyle)
            .extra_parser(CStressTestConfig::ExtraParser).run(),vm);
        po::notify(vm);
    }

    //----------------------------------------------------------------------------------------------------------
    CMthNetIp tIpAddr;
    if (!tIpAddr.SetIp(sLocalIpaddr))
    {
        cerr << "localipaddr error." << endl;
        return false;
    }
    iLocalIpType = tIpAddr.GetIpType();

    if (nTrustPort == 0)
    {
        nTrustPort = nListenBeginPort - 1;
    }
    string strDNSeedListenAddrV4 = "127.0.0.1";
    string strDNSeedListenAddrV6 = "::";

    tNetCfg.tListenEpIPV4.SetAddrPort(strDNSeedListenAddrV4, nTrustPort);
    tNetCfg.tListenEpIPV6.SetAddrPort(strDNSeedListenAddrV6, nTrustPort);
    nMagicNum = (fTestNet ? TESTNET_MAGICNUM : MAINNET_MAGICNUM);
    if (fDebug)
    {
        blockhead::STD_DEBUG = true;
    }

    return true;
}

void CStressTestConfig::ListConfig()
{
    cout << "help: " << (fHelp?"true":"false") << endl;
    cout << "testnet: " << (fTestNet?"true":"false") << endl;
    cout << "debug: " << (fDebug?"true":"false") << endl;
    cout << "workdir: " << sWorkDir << endl;
    cout << "trustport: " << nTrustPort << endl;
    cout << "localip: " << sLocalIpaddr << endl;
    cout << "beginport: " << nListenBeginPort << endl;
    cout << "portcount: " << nListenPortCount << endl;
    cout << "perportcount: " << nPerPortCount << endl;
    cout << "connectip: " << sConnectIpaddr << endl;
    cout << "connectport: " << nConnectPort << endl;
    cout << "perconnectcount: " << nPerConnectCount << endl;
    cout << "maxconnectcount: " << nMaxConnectCount << endl;
}

void CStressTestConfig::Help()
{
    cout << defaultDesc << endl;
}

bool CStressTestConfig::TokenizeConfile(const char *pzConfile,vector<string>& tokens)
{
    ifstream ifs(pzConfile);
    if (!ifs)
    {
        return false;
    }
    string line;
    while(!getline(ifs,line).eof() || !line.empty())
    {
        string s = line.substr(0,line.find('#'));
        boost::trim(s);
        if (!s.empty())
        {
            AddToken(s, tokens);
        }
        line.clear();
    }
    return true;
}

void CStressTestConfig::AddToken(string& sIn, vector<string>& tokens)
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
    sTempValue = sIn.substr(pos1+1, -1);
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

        sTempStr = sTempValue.substr(pos1+1, -1);
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

pair<string,string> CStressTestConfig::ExtraParser(const string& s)
{
    if (s[0] == '-' && !isdigit(s[1]))
    {
        bool fRev = (s.substr(1,2) == "no");
        size_t eq = s.find('=');
        if (eq == string::npos)
        {
            if (fRev)
            {
                return make_pair(s.substr(3),string("0"));
            }
            else
            {
                return make_pair(s.substr(1),string("1"));
            }
        }
        else if (fRev)
        {
            int v = atoi(s.substr(eq+1).c_str());
            return make_pair(s.substr(3,eq-3),string(v != 0 ? "0" : "1"));
        }
    }
    return make_pair(string(), string());
}


//---------------------------------------------------------------------------
CStressTestWorkThreadPool::CStressTestWorkThreadPool(CStressTestConfig *pCfg, CNetWorkService *nws,CStressTestBbAddrPool *pBbAddrPoolIn) : 
    pTestCfg(pCfg), pNetWorkService(nws), pBbAddrPool(pBbAddrPoolIn)
{
    nWorkThreadCount = pNetWorkService->GetWorkThreadCount();
    if (nWorkThreadCount > MAX_NET_WORK_THREAD_COUNT)
    {
        nWorkThreadCount = MAX_NET_WORK_THREAD_COUNT;
    }

    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        pWorkThreadTable[i] = new CStressTestMsgWorkThread(nWorkThreadCount, i, pCfg, nws, pBbAddrPoolIn);
    }
}

CStressTestWorkThreadPool::~CStressTestWorkThreadPool()
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        delete pWorkThreadTable[i];
        pWorkThreadTable[i] = NULL;
    }
}

bool CStressTestWorkThreadPool::StartAll()
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

void CStressTestWorkThreadPool::StopAll()
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        if (pWorkThreadTable[i])
        {
            pWorkThreadTable[i]->Stop();
        }
    }
}

bool CStressTestWorkThreadPool::StatData(CStressTestStat& tStat)
{
    for (uint32 i=0; i<nWorkThreadCount; i++)
    {
        if (pWorkThreadTable[i])
        {
            CStressTestStat tTempStat;
            pWorkThreadTable[i]->GetStatData(tTempStat);
            tStat += tTempStat;
        }
    }
    return true;
}


//-----------------------------------------------------------------------------------------------
CStressTestMsgWorkThread::CStressTestMsgWorkThread(uint32 nThreadCount, uint32 nThreadIndex, CStressTestConfig *pCfg, CNetWorkService *nws, CStressTestBbAddrPool *pBbAddrPoolIn) : 
    nWorkThreadCount(nThreadCount), nWorkThreadIndex(nThreadIndex), pTestCfg(pCfg), pNetWorkService(nws), pBbAddrPool(pBbAddrPoolIn), 
    pThreadMsgWork(NULL),pNetDataQueue(NULL),fRunFlag(false),tmPrevTimerDoTime(0)
{
    pNetDataQueue = &(nws->GetWorkRecvQueue(nThreadIndex));

    if (nThreadCount > 1)
    {
        if (pCfg->nPerConnectCount < nThreadCount)
        {
            nPersCalloutAddrCount = 1;
        }
        else
        {
            nPersCalloutAddrCount = pCfg->nPerConnectCount / nThreadCount;
            if (nThreadIndex == nThreadCount - 1)
            {
                nPersCalloutAddrCount += pCfg->nPerConnectCount % nThreadCount;
            }
        }
    }
    else
    {
        nPersCalloutAddrCount = pCfg->nPerConnectCount;
    }

    nStartTestTime = 0;
    nCompTestCount = 0;
}

CStressTestMsgWorkThread::~CStressTestMsgWorkThread()
{
    Stop();
}

bool CStressTestMsgWorkThread::Start()
{
    fRunFlag = true;
    pThreadMsgWork = new boost::thread(boost::bind(&CStressTestMsgWorkThread::Work, this));
    if (pThreadMsgWork == NULL)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Error msg work thread start.");
        return false;
    }
    return true;
}

void CStressTestMsgWorkThread::Stop()
{
    if (pThreadMsgWork)
    {
        fRunFlag = false;
        pThreadMsgWork->join();

        delete pThreadMsgWork;
        pThreadMsgWork = NULL;
    }
}

bool CStressTestMsgWorkThread::GetStatData(CStressTestStat& tStat)
{
    boost::unique_lock<boost::shared_mutex> lock(lockStat);
    tStat = tNsStatData;
    return true;
}


//-------------------------------------------------------------------------------
void CStressTestMsgWorkThread::Work()
{
    nStartTestTime = GetTimeMillis();

    while (fRunFlag)
    {
        Timer();

        CMthNetPackData *pPackData = NULL;
        if (!pNetDataQueue->GetData(pPackData, 100) || pPackData == NULL)
        {
            continue;
        }
        StressTestDoPacket(pPackData);
        delete pPackData;
    }
}

void CStressTestMsgWorkThread::Timer()
{
    time_t tmCurTime = time(NULL);
    if (tmCurTime - tmPrevTimerDoTime >= 1 || tmCurTime < tmPrevTimerDoTime)
    {
        tmPrevTimerDoTime = tmCurTime;
        StressTestDoPeerState(tmCurTime);
    }
    StressTestDoCallConnect();
}


//-------------------------------------------------------------------------------
void CStressTestMsgWorkThread::StressTestDoPacket(CMthNetPackData *pPackData)
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

        if (pPackData->epLocalAddr.GetPort() == pTestCfg->nTrustPort)
        {
            pTestCfg->fRecvTrustConnect = true;
        }

        if (StressTestAddNetPeer(true,pPackData->ui64NetId,pPackData->epPeerAddr,pPackData->epLocalAddr) == NULL)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "StressTestDoPacket NET_MSG_TYPE_SETUP StressTestAddNetPeer error.");
            pNetWorkService->RemoveNetPort(pPackData->ui64NetId);
            break;
        }
        break;
    }
    case NET_MSG_TYPE_DATA:
    {
        CStressTestNetPeer *pNetPeer = StressTestGetNetPeer(pPackData->ui64NetId);
        if (pNetPeer)
        {
            if (!pNetPeer->StressTestDoRecvPacket(pPackData))
            {
                StressTestActiveClosePeer(pPackData->ui64NetId);
                break;
            }
        }
        break;
    }
    case NET_MSG_TYPE_CLOSE:
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
        StressTestDelNetPeer(pPackData->ui64NetId);
        break;
    }
    case NET_MSG_TYPE_COMPLETE_NOTIFY:
    {
        if (pPackData->eDisCause != NET_DIS_CAUSE_CONNECT_SUCCESS)
        {
            string sErrorInfo = string("Connect peer fail, peer: ") + pPackData->epPeerAddr.ToString();
            blockhead::StdDebug("CFLOW", sErrorInfo.c_str());

            boost::unique_lock<boost::shared_mutex> lock(lockStat);
            tNsStatData.nTotalOutFailCount++;
            break;
        }
        string sInfo = string("Connect peer success, peer: ") + pPackData->epPeerAddr.ToString();
        blockhead::StdDebug("CFLOW", sInfo.c_str());

        CStressTestNetPeer* pNetPeer = StressTestAddNetPeer(false, pPackData->ui64NetId, pPackData->epPeerAddr, pPackData->epLocalAddr);
        if (pNetPeer == NULL)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "StressTestDoPacket NET_MSG_TYPE_COMPLETE_NOTIFY StressTestAddNetPeer error.");
            StressTestActiveClosePeer(pPackData->ui64NetId);
            break;
        }
        if (!pNetPeer->StressTestDoOutBoundConnectSuccess(pPackData->epLocalAddr))
        {
            StressTestActiveClosePeer(pPackData->ui64NetId);
            break;
        }
        break;
    }
    }
}

bool CStressTestMsgWorkThread::StressTestSendDataPacket(uint64 nNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn, char *pBuf, uint32 nLen)
{
    if (nNetId == 0 || pBuf == NULL || nLen == 0)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Param error.");
        return false;
    }
    CMthNetPackData *pPackData = new CMthNetPackData(nNetId, NET_MSG_TYPE_DATA, NET_DIS_CAUSE_UNKNOWN, tPeerEpIn, tLocalEpIn, pBuf, nLen);
    return pNetWorkService->ReqSendData(pPackData);
}

CStressTestNetPeer* CStressTestMsgWorkThread::StressTestAddNetPeer(bool fInBoundIn, uint64 nPeerNetId, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn)
{
    if (StressTestGetNetPeer(nPeerNetId) == NULL)
    {
        CStressTestNetPeer *pNetPeer = new CStressTestNetPeer(this, pBbAddrPool, pTestCfg->nMagicNum, 
            fInBoundIn, nPeerNetId, tPeerEpIn, tLocalEpIn);
        if (mapPeer.insert(make_pair(nPeerNetId,pNetPeer)).second)
        {
            boost::unique_lock<boost::shared_mutex> lock(lockStat);
            if (fInBoundIn)
            {
                tNsStatData.nCurInBoundCount++;
                tNsStatData.nTotalInConnectCount++;
            }
            else
            {
                tNsStatData.nCurOutBoundCount++;
                tNsStatData.nTotalOutConnectCount++;
                tNsStatData.nTotalOutSuccessCount++;
            }
            tNsStatData.nCurConnectCount++;
            return pNetPeer;
        }
        else
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "mapPeer insert fail.");
        }
    }
    return NULL;
}

void CStressTestMsgWorkThread::StressTestDelNetPeer(uint64 nPeerNetId)
{
    map<uint64, CStressTestNetPeer*>::iterator it;
    it = mapPeer.find(nPeerNetId);
    if (it != mapPeer.end())
    {
        CStressTestNetPeer *pPeer = it->second;
        mapPeer.erase(it);
        if (pPeer)
        {
            {
                boost::unique_lock<boost::shared_mutex> lock(lockStat);
                if (pPeer->fInBound)
                {
                    tNsStatData.nCurInBoundCount--;
                }
                else
                {
                    tNsStatData.nCurOutBoundCount--;
                }
                tNsStatData.nCurConnectCount--;
            }
            delete pPeer;
        }
    }
}

CStressTestNetPeer* CStressTestMsgWorkThread::StressTestGetNetPeer(uint64 nPeerNetId)
{
    map<uint64, CStressTestNetPeer*>::iterator it = mapPeer.find(nPeerNetId);
    if (it != mapPeer.end())
    {
        return it->second;
    }
    return NULL;
}

void CStressTestMsgWorkThread::StressTestActiveClosePeer(uint64 nPeerNetId)
{
    blockhead::StdDebug("CFLOW", "Active close connect");
    pNetWorkService->RemoveNetPort(nPeerNetId);
    StressTestDelNetPeer(nPeerNetId);
}

bool CStressTestMsgWorkThread::StressTestHandlePeerHandshaked(CStressTestNetPeer *pPeer)
{
    tcp::endpoint ep;
    pPeer->tPeerEp.GetEndpoint(ep);
    pBbAddrPool->StressTestUpdateNetTime(ep.address(), pPeer->nTimeDelta);
    pBbAddrPool->StressTestUpdateHeight(pPeer->tPeerEp, pPeer->nStartingHeight);
    return true;
}

void CStressTestMsgWorkThread::StressTestDoCallConnect()
{
    /*if (!pTestCfg->fRecvTrustConnect)
    {
        return;
    }*/

    int64 nTestTimeLen = GetTimeMillis() - nStartTestTime;
    int64 nNeedTestCount = nPersCalloutAddrCount * nTestTimeLen / 1000 - nCompTestCount;
    if (nNeedTestCount <= 0)
    {
        return ;
    }

    vector<CBbAddr> vBbAddr;
    if (!pBbAddrPool->StressTestGetCallConnectAddrList(vBbAddr, nNeedTestCount))
    {
        nStartTestTime = GetTimeMillis();
        nCompTestCount = 0;
        return;
    }

    vector<CBbAddr>::iterator it;
    for (it = vBbAddr.begin(); it != vBbAddr.end(); it++)
    {
        if (StressTestStartConnectPeer(*it))
        {
            nCompTestCount++;
        }
    }
}

void CStressTestMsgWorkThread::StressTestDoPeerState(time_t tmCurTime)
{
    CStressTestNetPeer* pNetPeer;
    uint32 nInCount = 0;
    uint32 nOutCount = 0;

    map<uint64, CStressTestNetPeer*>::iterator it;
    for (it = mapPeer.begin(); it != mapPeer.end(); )
    {
        if (it->second)
        {
            pNetPeer = it->second;
            it++;

            if (pNetPeer->fInBound) nInCount++;
            else nOutCount++;

            if(!pNetPeer->StressTestDoStateTimer(tmCurTime))
            {
                StressTestActiveClosePeer(pNetPeer->nPeerNetId);
            }
        }
        else
        {
            it++;
        }
    }

    {
        boost::unique_lock<boost::shared_mutex> lock(lockStat);
        tNsStatData.nCurConnectCount = nInCount + nOutCount;
        tNsStatData.nCurInBoundCount = nInCount;
        tNsStatData.nCurOutBoundCount = nOutCount;
    }
}

bool CStressTestMsgWorkThread::StressTestStartConnectPeer(CBbAddr& tBbAddr)
{
    if (blockhead::STD_DEBUG)
    {
        string sInfo = string("Start connect peer, peer: ") + tBbAddr.GetEp().ToString();
        blockhead::StdDebug("CFLOW", sInfo.c_str());
    }

    uint64 nOutNetId = 0;
    if (!pNetWorkService->ReqTcpConnect(tBbAddr.GetEp(), nOutNetId))
    {
        string sInfo = string("ReqTcpConnect fail, peer: ") + tBbAddr.GetEp().ToString();
        blockhead::StdError(__PRETTY_FUNCTION__, sInfo.c_str());
        return false;
    }

    return true;
}


//-----------------------------------------------------------------------------------------------
CStressTestNetPeer::CStressTestNetPeer(CStressTestMsgWorkThread *pMsgWorkThreadIn, CStressTestBbAddrPool *pBbAddrPoolIn, uint32 nMsgMagicIn, 
    bool fInBoundIn, uint64 nNetIdIn, CMthNetEndpoint& tPeerEpIn, CMthNetEndpoint& tLocalEpIn) : 
    pMsgWorkThread(pMsgWorkThreadIn),pBbAddrPool(pBbAddrPoolIn),nMsgMagic(nMsgMagicIn),
    fInBound(fInBoundIn),nPeerNetId(nNetIdIn),tPeerEp(tPeerEpIn),tLocalEp(tLocalEpIn)
{
    nVersion = 0;
    nService = 0;
    nNonceFrom = 0;
    nTimeDelta = 0;
    nSendHelloTime = 0;
    nStartingHeight = 0;
    fGetPeerAddress = false;
    fIfNeedRespGetAddress = false;

    if (fInBoundIn)
    {
        StressTestModifyPeerState(DNP_E_ST_PEER_STATE_IN_CONNECTED);
    }
    else
    {
        StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_CONNECTING);
    }
}

CStressTestNetPeer::~CStressTestNetPeer()
{
}

bool CStressTestNetPeer::StressTestDoRecvPacket(CMthNetPackData *pPackData)
{
    if (pPackData == NULL || pPackData->GetDataBuf() == NULL || pPackData->GetDataLen() == 0 )
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Param error.");
        return false;
    }
    tRecvDataBuf += *pPackData;

    while (tRecvDataBuf.CheckPacketIntegrity())
    {
        if (!tRecvDataBuf.VerifyPacket(nMsgMagic))
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "VerifyPacket fail.");
            return false;
        }
        if (tRecvDataBuf.GetChannel() == BBPROTO_CHN_NETWORK)
        {
            if (!StressTestDoPeerPacket())
            {
                return false;
            }
        }
        tRecvDataBuf.ErasePacket();
    }
    return true;
}

bool CStressTestNetPeer::StressTestDoOutBoundConnectSuccess(CMthNetEndpoint& tLocalEp)
{
    tLocalEp = tLocalEp;
    StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_CONNECTED);
    if (!StressTestSendMsgHello())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Send hello message fail.");
        return false;
    }
    StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_WAIT_HELLO);
    return true;
}


//------------------------------------------------------------------------------------
bool CStressTestNetPeer::StressTestDoPeerPacket()
{
    int64 nTimeRecv = GetTime();
    switch (tRecvDataBuf.GetCommand())
    {
    case BBPROTO_CMD_HELLO:
    {
        try{
            blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_HELLO");
            if (nVersion != 0)
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "nVersion error.");
                return false;
            }

            CBlockheadBufStream ssPayload;
            if (!tRecvDataBuf.GetPayload(ssPayload))
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "GetPayload fail.");
                return false;
            }

            int64 nTime;
            ssPayload >> nVersion >> nService >> nTime >> nNonceFrom >> strSubVer >> nStartingHeight;
            nTimeDelta = nTime - nTimeRecv;

            if (STD_DEBUG)
            {
                string sInfo = string("hello peer: ") + tPeerEp.ToString() + string(", height: ") + to_string(nStartingHeight);
                blockhead::StdDebug("CFLOW", sInfo.c_str());
            }

            if (fInBound)
            {
                if (ePeerState == DNP_E_ST_PEER_STATE_IN_CONNECTED)
                {
                    if (!StressTestSendMsgHello())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "StressTestSendMsgHello fail.");
                        return false;
                    }
                    StressTestModifyPeerState(DNP_E_ST_PEER_STATE_IN_WAIT_HELLO_ACK);
                }
                else
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO in state error.");
                    return false;
                }
            }
            else
            {
                if (ePeerState == DNP_E_ST_PEER_STATE_OUT_WAIT_HELLO)
                {
                    StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_HANDSHAKED_COMPLETE);
                    nTimeDelta += (nTimeRecv - nSendHelloTime) / 2;
                    pMsgWorkThread->StressTestHandlePeerHandshaked(this);

                    if (!StressTestSendMsgHelloAck())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "StressTestSendMsgHelloAck fail.");
                        return false;
                    }
                    if (!StressTestSendMsgGetAddress())
                    {
                        blockhead::StdError(__PRETTY_FUNCTION__, "StressTestSendMsgGetAddress fail.");
                        return false;
                    }
                    StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_WAIT_ADDRESS_RSP);
                }
                else
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO out state error.");
                    return false;
                }
            }
        }
        catch (exception& e)
        {
            blockhead::StdError(__PRETTY_FUNCTION__, e.what());
            return false;
        }
        break;
    }
    case BBPROTO_CMD_HELLO_ACK:
    {
        blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_HELLO_ACK");
        if (fInBound)
        {
            if (ePeerState == DNP_E_ST_PEER_STATE_IN_WAIT_HELLO_ACK)
            {
                if (nVersion == 0)
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "nVersion error.");
                    return false;
                }
                StressTestModifyPeerState(DNP_E_ST_PEER_STATE_IN_HANDSHAKED_COMPLETE);
                nTimeDelta += (nTimeRecv - nSendHelloTime) / 2;
                pMsgWorkThread->StressTestHandlePeerHandshaked(this);
                if (!StressTestSendMsgGetAddress())
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "StressTestSendMsgGetAddress fail.");
                    return false;
                }
                StressTestModifyPeerState(DNP_E_ST_PEER_STATE_IN_WAIT_ADDRESS_RSP);
            }
            else
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_HELLO_ACK in state error.");
                return false;
            }
        }
        break;
    }
    case BBPROTO_CMD_GETADDRESS:
    {
        blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_GETADDRESS");
        if (!StressTestSendMsgAddress())
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "StressTestSendMsgAddress fail.");
            return false;
        }
        break;
    }
    case BBPROTO_CMD_ADDRESS:
    {
        blockhead::StdDebug("CFLOW", "Recv msg: BBPROTO_CMD_ADDRESS");

        vector<CAddress> vAddrList;
        CBlockheadBufStream ssPayload;
        if (!tRecvDataBuf.GetPayload(ssPayload))
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "GetPayload fail.");
            return false;
        }
        ssPayload >> vAddrList;

        if (STD_DEBUG)
        {
            string sInfo = string("Recv address count: ") + to_string(vAddrList.size());
            blockhead::StdDebug("CFLOW", sInfo.c_str());
        }

        for (vector<CAddress>::iterator it = vAddrList.begin(); it != vAddrList.end(); ++it)
        {
            if (((*it).nService & NODE_NETWORK) == NODE_NETWORK)
            {
                tcp::endpoint ep;
                (*it).ssEndpoint.GetEndpoint(ep);
                pBbAddrPool->StressTestAddRecvAddr(ep, (*it).nService);

                if (STD_DEBUG)
                {
                    string strAddAddr = string("Address: ") + ep.address().to_string() + ":" + to_string(ep.port());
                    blockhead::StdDebug("CFLOW", strAddAddr.c_str());
                }
            }
        }
        fGetPeerAddress = true;
        
        if (ePeerState == DNP_E_ST_PEER_STATE_IN_WAIT_ADDRESS_RSP)
        {
            StressTestModifyPeerState(DNP_E_ST_PEER_STATE_IN_COMPLETE);
        }
        else if (ePeerState == DNP_E_ST_PEER_STATE_OUT_WAIT_ADDRESS_RSP)
        {
            StressTestModifyPeerState(DNP_E_ST_PEER_STATE_OUT_COMPLETE);
        }
        else
        {
            blockhead::StdError(__PRETTY_FUNCTION__, "BBPROTO_CMD_ADDRESS state error.");
            return false;
        }
        break;
    }
    default:
        blockhead::StdError(__PRETTY_FUNCTION__, "Error message.");
        break;
    }

    return true;
}


void CStressTestNetPeer::StressTestModifyPeerState(DNP_E_PEER_STATE eState)
{
    ePeerState = eState;
    tmStateBeginTime = time(NULL);
}

bool CStressTestNetPeer::StressTestSendMessage(int nChannel, int nCommand, CBlockheadBufStream& ssPayload)
{
    CProtoDataBuf tPacketBuf(nMsgMagic, nChannel, nCommand, ssPayload);
    return pMsgWorkThread->StressTestSendDataPacket(nPeerNetId, tPeerEp, tLocalEp, tPacketBuf.GetDataBuf(), tPacketBuf.GetDataLen());
}

bool CStressTestNetPeer::StressTestDoStateTimer(time_t tmCurTime)
{
    switch (ePeerState)
    {
    case DNP_E_ST_PEER_STATE_INIT:
        break;

    case DNP_E_ST_PEER_STATE_IN_CONNECTED:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 30))
        {
            blockhead::StdDebug("CFLOW", "In connect timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_IN_WAIT_HELLO_ACK:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "In connect wait hello ack timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_IN_HANDSHAKED_COMPLETE:
        break;
    case DNP_E_ST_PEER_STATE_IN_WAIT_ADDRESS_RSP:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "In connect wait address rsp timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_IN_COMPLETE:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 2))
        {
            blockhead::StdDebug("CFLOW", "In connect work complete.");
            return false;
        }
        break;

    case DNP_E_ST_PEER_STATE_OUT_CONNECTING:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "Out connecting timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_OUT_CONNECTED:
        break;
    case DNP_E_ST_PEER_STATE_OUT_WAIT_HELLO:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "Out connect wait hello timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_OUT_HANDSHAKED_COMPLETE:
        break;
    case DNP_E_ST_PEER_STATE_OUT_WAIT_ADDRESS_RSP:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 20))
        {
            blockhead::StdDebug("CFLOW", "Out connect wait address rsp timeout.");
            return false;
        }
        break;
    case DNP_E_ST_PEER_STATE_OUT_COMPLETE:
        if (DNP_ST_STATE_TIMEOUT(tmCurTime, 2))
        {
            blockhead::StdDebug("CFLOW", "Out connect work complete.");
            return false;
        }
        break;
    }
    return true;
}


//-----------------------------------------------------------------------------------
bool CStressTestNetPeer::StressTestSendMsgHello()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_HELLO");
    CBlockheadBufStream ssPayload;
    int iLocalVersion = PROTO_VERSION;
    uint64 nLocalService = NODE_NETWORK;
    string strLocalSubVer = FormatSubVersion();

    uint64 nNonce = nPeerNetId;
    int64 nTime = pBbAddrPool->StressTestGetNetTime();
    int nHeight = pBbAddrPool->StressTestGetConfidentHeight();
    ssPayload << iLocalVersion << nLocalService << nTime << nNonce << strLocalSubVer << nHeight; 

    nSendHelloTime = GetTime();
    return StressTestSendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_HELLO, ssPayload);
}

bool CStressTestNetPeer::StressTestSendMsgHelloAck()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_HELLO_ACK");
    CBlockheadBufStream ssPayload;
    return StressTestSendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_HELLO_ACK, ssPayload);
}

bool CStressTestNetPeer::StressTestSendMsgGetAddress()
{
    blockhead::StdDebug("CFLOW", "Send msg: BBPROTO_CMD_GETADDRESS");
    CBlockheadBufStream ssPayload;
    return StressTestSendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_GETADDRESS, ssPayload);
}

bool CStressTestNetPeer::StressTestSendMsgAddress()
{
    CBlockheadBufStream ssPayload;
    vector<CAddress> vAddrList;
    pBbAddrPool->StressTestGetGoodAddressList(vAddrList);

    string sInfo = string("Send msg: BBPROTO_CMD_ADDRESS, addr count: ") + to_string(vAddrList.size());
    blockhead::StdDebug("CFLOW", sInfo.c_str());

    ssPayload << vAddrList;
    return StressTestSendMessage(BBPROTO_CHN_NETWORK, BBPROTO_CMD_ADDRESS, ssPayload);
}



//-------------------------------------------------------------------------------------
CStressTestBbAddrPool::CStressTestBbAddrPool(CStressTestConfig *pCfg, CNetWorkService *pNetWS) : 
    pTestCfg(pCfg),pNetWorkService(pNetWS)
{
    nCurConnectCount = 0;
    nCurGetPortPos = 0;
}

CStressTestBbAddrPool::~CStressTestBbAddrPool()
{

}

bool CStressTestBbAddrPool::StressTestStartAllTcpListen()
{
    if (pTestCfg->nListenPortCount == 0 || pTestCfg->nListenPortCount > HSS_MAX_STRESS_LISTEN_PORT_COUNT)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "nPortCount error.");
        return false;
    }

    if (pTestCfg->nListenBeginPort == 0)
    {
        uint32 nListenCount = 0;

        while (nListenCount < pTestCfg->nListenPortCount)
        {
            uint64 nListenNetId;
            if (pTestCfg->iLocalIpType == CMthNetIp::MNI_IPV4)
            {
                //Start ipv4 listen
                if ((nListenNetId=pNetWorkService->AddTcpIPV4Listen(0)) == 0)
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "Start ipv4 tcp listen fail, port: 0");
                    return false;
                }
            }
            else if (pTestCfg->iLocalIpType == CMthNetIp::MNI_IPV6)
            {
                //Start ipv6 listen
                if ((nListenNetId=pNetWorkService->AddTcpIPV6Listen(0)) == 0)
                {
                    blockhead::StdError(__PRETTY_FUNCTION__, "Start ipv6 tcp listen fail, port: 0");
                    return false;
                }
            }
            else
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "Local ipaddr type error.");
                return false;
            }

            CMthNetEndpoint tListenAddr;
            if (!pNetWorkService->QueryTcpListenAddress(nListenNetId, tListenAddr))
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "QueryTcpListenAddress fail.");
                return false;
            }
            nListenPortTable[nListenCount++] = tListenAddr.GetPort();
        }
    }
    else
    {
        uint32 nListenCount = 0;
        uint16 nListenPort = pTestCfg->nListenBeginPort;

        while (nListenCount < pTestCfg->nListenPortCount)
        {
            if (pTestCfg->iLocalIpType == CMthNetIp::MNI_IPV4)
            {
                //Start ipv4 listen
                if (pNetWorkService->AddTcpIPV4Listen(nListenPort) == 0)
                {
                    string sInfo = string("Start ipv4 tcp listen fail, port: ") + to_string(nListenPort);
                    blockhead::StdError(__PRETTY_FUNCTION__, sInfo.c_str());
                    //return false;
                }
            }
            else if (pTestCfg->iLocalIpType == CMthNetIp::MNI_IPV6)
            {
                //Start ipv6 listen
                if (pNetWorkService->AddTcpIPV6Listen(nListenPort) == 0)
                {
                    string sInfo = string("Start ipv6 tcp listen fail, port: ") + to_string(nListenPort);
                    blockhead::StdError(__PRETTY_FUNCTION__, sInfo.c_str());
                    //return false;
                }
            }
            else
            {
                blockhead::StdError(__PRETTY_FUNCTION__, "Local ipaddr type error.");
                return false;
            }
            nListenPortTable[nListenCount++] = nListenPort++;
        }
    }

    return true;
}

bool CStressTestBbAddrPool::StressTestAddRecvAddr(tcp::endpoint& ep, uint64 nServiceIn)
{
    return true;
}

bool CStressTestBbAddrPool::StressTestUpdateHeight(CMthNetEndpoint &ep, int iHeight)
{
    return true;
}

void CStressTestBbAddrPool::StressTestGetGoodAddressList(vector<CAddress>& vAddrList)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);

    uint64 nService = NODE_NETWORK;

    for (int i=0; i<pTestCfg->nPerPortCount; i++)
    {
        uint16 nListenPort = nListenPortTable[nCurGetPortPos];
        nCurGetPortPos = (nCurGetPortPos + 1) % pTestCfg->nListenPortCount;

        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::address::from_string(pTestCfg->sLocalIpaddr), nListenPort);
        vAddrList.push_back(CAddress(nService, ep));
    }
}

bool CStressTestBbAddrPool::StressTestGetCallConnectAddrList(vector<CBbAddr>& vBbAddr, uint32 nGetAddrCount)
{
    boost::unique_lock<boost::shared_mutex> lock(lockAddrPool);

    if (nCurConnectCount >= pTestCfg->nMaxConnectCount)
    {
        return false;
    }

    uint64 nService = NODE_NETWORK;
    CBbAddr tAddr;
    tAddr.SetBbAddr(pTestCfg->sConnectIpaddr, pTestCfg->nConnectPort, nService, 0);

    for (int i=0; i<nGetAddrCount; i++)
    {
        vBbAddr.push_back(CBbAddr(tAddr));

        if (++nCurConnectCount >= pTestCfg->nMaxConnectCount)
        {
            break;
        }
    }

    return true;
}

int64 CStressTestBbAddrPool::StressTestGetNetTime()
{
    return GetTime();
}

bool CStressTestBbAddrPool::StressTestUpdateNetTime(const boost::asio::ip::address& address,int64 nTimeDelta)
{
    return true;
}

void CStressTestBbAddrPool::StressTestSetConfidentHeight(uint32 nHeight)
{

}

uint32 CStressTestBbAddrPool::StressTestGetConfidentHeight()
{
    return 1000;
}


//--------------------------------------------------------------------------------------
CStressTestService::CStressTestService()
{
    pNetWorkService = NULL;
    pBbAddrPool = NULL;
    pWorkThreadPool = NULL;
    pThreadWork = NULL;
    fRunFlag = false;
}

CStressTestService::~CStressTestService()
{
    if (pNetWorkService)
    {
        delete pNetWorkService;
        pNetWorkService = NULL;
    }
    if (pBbAddrPool)
    {
        delete pBbAddrPool;
        pBbAddrPool = NULL;
    }
    if (pWorkThreadPool)
    {
        delete pWorkThreadPool;
        pWorkThreadPool = NULL;
    }
}

bool CStressTestService::StartService(int argc, char *argv[])
{
    if (!tStressTestCfg.ReadConfig(argc, argv, GetDefaultDataDir(), "dnseedstresstest.conf"))
    {
        cerr << "Failed read config.\n";
        return false;
    }

    // help
    if (tStressTestCfg.fHelp)
    {
        tStressTestCfg.Help();
        return false;
    }

    // list config if in debug mode
    if (tStressTestCfg.fDebug)
    {
        tStressTestCfg.ListConfig();
    }

    // path
    boost::filesystem::path& pathData = tStressTestCfg.pathData;
    if (!exists(pathData))
    {
        if (!create_directories(pathData))
        {
            cerr << "Failed create directory : " << pathData << "\n";
            return false;
        }
    }

    if (!is_directory(pathData))
    {
        cerr << "Failed to access data directory : " << pathData << "\n";
        return false;
    }

    //--------------------------------------------------------------------------------
    pNetWorkService = new CNetWorkService();
    pBbAddrPool = new CStressTestBbAddrPool(&tStressTestCfg, pNetWorkService);
    pWorkThreadPool = new CStressTestWorkThreadPool(&tStressTestCfg, pNetWorkService, pBbAddrPool);

    if (pNetWorkService->AddTcpListen(tStressTestCfg.tNetCfg.tListenEpIPV4) == 0)
    {
        cerr << "Failed to add ipv4 tcp listen.\n";
        return false;
    }
    if (pNetWorkService->AddTcpListen(tStressTestCfg.tNetCfg.tListenEpIPV6) == 0)
    {
        cerr << "Failed to add ipv6 tcp listen.\n";
        return false;
    }

    if (!pNetWorkService->StartService())
    {
        StopService();
        return false;
    }

    if (!pBbAddrPool->StressTestStartAllTcpListen())
    {
        StopService();
        return false;
    }

    if (!pWorkThreadPool->StartAll())
    {
        StopService();
        return false;
    }

    if (!Start())
    {
        StopService();
        return false;
    }

    return true;
}

void CStressTestService::StopService()
{
    Stop();

    if (pWorkThreadPool)
    {
        pWorkThreadPool->StopAll();
    }
    if (pNetWorkService)
    {
        pNetWorkService->StopService();
    }
    
    if (pNetWorkService)
    {
        delete pNetWorkService;
        pNetWorkService = NULL;
    }
    if (pBbAddrPool)
    {
        delete pBbAddrPool;
        pBbAddrPool = NULL;
    }
    if (pWorkThreadPool)
    {
        delete pWorkThreadPool;
        pWorkThreadPool = NULL;
    }
}

boost::filesystem::path CStressTestService::GetDefaultDataDir()
{
    // Windows: C:\Documents and Settings\username\Local Settings\Application Data\bigdnseed
    // Mac: ~/Library/Application Support/bigdnseed
    // Unix: ~/.bigdnseed

#ifdef WIN32
    // Windows
    char pszPath[MAX_PATH] = "";
    if (SHGetSpecialFolderPathA(NULL, pszPath, CSIDL_LOCAL_APPDATA, true))
    {
        return path(pszPath) / "bigdnseed";
    }
    return path("C:\\bigdnseed");
#else
    boost::filesystem::path pathRet;
    char *pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
    {
        pathRet = boost::filesystem::path("/");
    }
    else
    {
        pathRet = boost::filesystem::path(pszHome);
    }
#ifdef __APPLE__
    // Mac
    pathRet /= "Library/Application Support";
    create_directory(pathRet);
    return pathRet / "bigdnseed";
#else
    // Unix
    return pathRet / ".bigdnseed";
#endif
#endif
}

bool CStressTestService::Start()
{
    fRunFlag = true;
    pThreadWork = new boost::thread(boost::bind(&CStressTestService::Work, this));
    if (pThreadWork == NULL)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Start thread fail.");
        return false;
    }
    return true;
}

void CStressTestService::Stop()
{
    fRunFlag = false;
    if (pThreadWork)
    {
        pThreadWork->join();
        delete pThreadWork;
        pThreadWork = NULL;
    }
}

void CStressTestService::Work()
{
    time_t tmBeginTime = time(NULL);
    tmPrevStatTime = tmBeginTime;
    while (fRunFlag)
    {
        sleep(1);
        if (time(NULL) - tmBeginTime >= 10)
        {
            tmBeginTime = time(NULL);
            Stat();
        }
    }
}

void CStressTestService::Stat()
{
    CStressTestStat tStatData;
    time_t tmCurTime = time(NULL);
    int iStatLen = tmCurTime - tmPrevStatTime;
    tmPrevStatTime = tmCurTime;
    if (iStatLen <= 0)
        iStatLen = 1;

    pWorkThreadPool->StatData(tStatData);

    char buf[128] = {0};

    blockhead::StdLog("STAT", "======================================================");

    sprintf(buf, "Current connect count: %d", tStatData.nCurConnectCount);
    blockhead::StdLog("STAT", buf);
    sprintf(buf, "Current in count: %d", tStatData.nCurInBoundCount);
    blockhead::StdLog("STAT", buf);
    sprintf(buf, "Current out count: %d", tStatData.nCurOutBoundCount);
    blockhead::StdLog("STAT", buf);

    sprintf(buf, "Total out count: %d - %d", tStatData.nTotalOutConnectCount, (tStatData.nTotalOutConnectCount - tPrevStatData.nTotalOutConnectCount) / iStatLen);
    blockhead::StdLog("STAT", buf);
    sprintf(buf, "Out success: %d - %d", tStatData.nTotalOutSuccessCount, (tStatData.nTotalOutSuccessCount - tPrevStatData.nTotalOutSuccessCount) / iStatLen);
    blockhead::StdLog("STAT", buf);
    sprintf(buf, "Out fail: %d - %d", tStatData.nTotalOutFailCount, (tStatData.nTotalOutFailCount - tPrevStatData.nTotalOutFailCount) / iStatLen);
    blockhead::StdLog("STAT", buf);
    sprintf(buf, "Total in count: %d - %d", tStatData.nTotalInConnectCount, (tStatData.nTotalInConnectCount - tPrevStatData.nTotalInConnectCount) / iStatLen);
    blockhead::StdLog("STAT", buf);

    tPrevStatData = tStatData;
}


/////////////////////////////////////////////////////////////////////////////////////////
// main_stress_test

int main_stress_test(int argc, char **argv)
{
    CStressTestService tStressTest;
    if (!tStressTest.StartService(argc, argv))
    {
        printf("StartService fail\n");
        tStressTest.StopService();
        return 0;
    }
    blockhead::StdLog("MAIN", "Start stress test success.");

    std::string strInput;
    while(1)
    {
        std::getline(std::cin, strInput);
        if (strInput == "quit")
            break;
    }

    tStressTest.StopService();
    blockhead::StdLog("MAIN", "Quit over.");

    return 0;
}

} //namespace stresstest

