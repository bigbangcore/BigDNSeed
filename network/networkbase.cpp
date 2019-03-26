// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkbase.h"

using namespace std;

namespace network
{

//----------------------------------------------------------------------------------------
static const unsigned char epipv4[12] = {0,0,0,0,0,0,0,0,0,0,0xff,0xff};

#define INET_ADDRSTRLEN  16  
#define INET6_ADDRSTRLEN 46  

bool CMthNetIp::ToByteIpaddr(const char *pIpStr, unsigned char *pIpByte, int *pIpType) // ip type: 4: ipv4, 6: ipv6
{
    if (pIpStr == NULL || pIpByte == NULL || pIpType == NULL)
    {
        return false;
    }

    struct in_addr ipv4Addr;
    if (inet_pton(AF_INET, pIpStr, (void*)&ipv4Addr) == 1)
    {
        memcpy(pIpByte, (void*)&ipv4Addr, 4);
        *pIpType = MNI_IPV4;
        return true;
    }
    
    struct in6_addr ipv6Addr;
    if (inet_pton(AF_INET6, pIpStr, (void*)&ipv6Addr) == 1)
    {
        memcpy(pIpByte, (void*)&ipv6Addr, 16);
        *pIpType = MNI_IPV6;
        return true;
    }
    
    return false;
}

bool CMthNetIp::ToStrIpaddr(const unsigned char *pIpByte, const int iIpType, char *pIpStrBuf, int iIpStrBufSize) // ip type: 4: ipv4, 6: ipv6
{
    if (pIpByte == NULL || pIpStrBuf == NULL)
    {
        return false;
    }

    if (iIpType == MNI_IPV4)
    {
        if (inet_ntop(AF_INET, pIpByte, pIpStrBuf, iIpStrBufSize) == NULL)
        {
            return false;
        }
    }
    else if (iIpType == MNI_IPV6)
    {
        if (inet_ntop(AF_INET6, pIpByte, pIpStrBuf, iIpStrBufSize) == NULL)
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    
    return true;
}

bool CMthNetIp::SplitHostPort(const char *pAddr, char *pHost, int iHostBufSize, unsigned short *pPort)
{
    char *pos, *findpos;
    int len;

    if (pAddr == NULL || pHost == NULL || iHostBufSize <= 0 || pPort == NULL)
    {
        return false;
    }

    pos = (char*)pAddr;
    while (*pos && *pos == ' ') pos++;
    if (*pos == 0) return false;

    if (*pos == '[')
    {
        pos++;
        findpos = strchr(pos, ']');
        if (findpos == NULL) return false;
        len = findpos - pos;
        if (len <= 0 || len >= iHostBufSize) return false;
        memcpy(pHost, pos, len);
        pHost[len] = 0;

        pos = findpos+1;
        while (*pos && *pos == ' ') pos++;
        if (*pos == 0)
        {
            *pPort = 0;
            return true;
        }
        if (*pos != ':') return false;

        pos++;
        while (*pos && *pos == ' ') pos++;
        if (*pos == 0)
            *pPort = 0;
        else
            *pPort = atoi(pos);
    }
    else
    {
        findpos = strchr(pos, ':');
        if (findpos == NULL)
        {
            len = strlen(pos);
            if (len <= 0 || len >= iHostBufSize) return false;
            memcpy(pHost, pos, len);
            pHost[len] = 0;
            *pPort = 0;
        }
        else
        {
            len = findpos - pos;
            if (len <= 0 || len >= iHostBufSize) return false;
            memcpy(pHost, pos, len);
            pHost[len] = 0;

            pos = findpos + 1;
            while (*pos && *pos == ' ') pos++;
            if (*pos == 0)
                *pPort = 0;
            else
                *pPort = atoi(pos);
        }
    }
    return true;
}


//--------------------------------------------------------------------------
CMthNetIp::CMthNetIp(const unsigned char *pIpByte, const int iIpTypeIn)
{
    memset(byteIp, 0, sizeof(byteIp));
    iIpType = MNI_UNKNOWN;
    if (pIpByte)
    {
        if (iIpTypeIn == MNI_IPV4)
        {
            memcpy(byteIp, pIpByte, 4);
            iIpType = iIpTypeIn;

            char buf[INET_ADDRSTRLEN] = {0};
            ToStrIpaddr(pIpByte, MNI_IPV4, buf, INET_ADDRSTRLEN);
            strIp = buf;
        }
        else if (iIpTypeIn == MNI_IPV6)
        {
            memcpy(byteIp, pIpByte, 16);
            iIpType = iIpTypeIn;

            char buf[INET6_ADDRSTRLEN] = {0};
            ToStrIpaddr(pIpByte, MNI_IPV6, buf, INET6_ADDRSTRLEN);
            strIp = buf;
        }
    }
}

CMthNetIp::CMthNetIp(const CMthNetIp& na)
{
    memcpy(byteIp, na.byteIp, 16);
    iIpType = na.iIpType;
    strIp = na.strIp;
}

bool CMthNetIp::SetIp(const char* pIpIn)
{
    if (pIpIn == NULL || pIpIn[0] == 0)
    {
        return false;
    }

    unsigned char ucTempIp[16] = {0};
    int iTempIpType = 0;
    if (!ToByteIpaddr(pIpIn, ucTempIp, &iTempIpType))
    {
        return false;
    }

    char ipbuf[64] = {0};
    if (!ToStrIpaddr(ucTempIp, iTempIpType, ipbuf, sizeof(ipbuf)))
    {
        return false;
    }

    memcpy(byteIp, ucTempIp, sizeof(byteIp));
    iIpType = iTempIpType;
    strIp = ipbuf;
    return true;
}

bool CMthNetIp::SetIp(const string& strIpIn)
{
    if (strIpIn.empty())
    {
        return false;
    }
    return SetIp(strIpIn.c_str());
}

string& CMthNetIp::GetIp()
{
    return strIp;
}


//----------------------------------------------------------------------------------------
bool CMthNetEndpoint::SetAddrPort(const char* pIpIn, const uint16 nPortIn)
{
    if (!SetIp(pIpIn))
    {
        return false;
    }
    usPort = nPortIn;
    return true;
}

bool CMthNetEndpoint::SetAddrPort(string& strIpIn, uint16 nPortIn)
{
    if (!SetIp(strIpIn))
    {
        return false;
    }
    usPort = nPortIn;
    return true;
}

bool CMthNetEndpoint::SetAddrPort(tcp::endpoint& ep)
{
    if (!SetIp(ep.address().to_string()))
    {
        return false;
    }
    usPort = ep.port();
    return true;
}

bool CMthNetEndpoint::SetEndpoint(tcp::endpoint& epAddr)
{
    if (!SetIp(epAddr.address().to_string()))
    {
        return false;
    }
    usPort = epAddr.port();
    return true;
}

bool CMthNetEndpoint::SetAddrPort(const char* pIpPort)
{
    char sHostBuf[INET6_ADDRSTRLEN] = {0};
    if (!SplitHostPort(pIpPort, sHostBuf, sizeof(sHostBuf), &usPort))
    {
        return false;
    }
    if (!SetIp(sHostBuf))
    {
        return false;
    }
    return true;
}

bool CMthNetEndpoint::SetAddrPort(string& strIpPort)
{
    return SetAddrPort(strIpPort.c_str());
}

bool CMthNetEndpoint::GetEndpoint(tcp::endpoint& epAddr)
{
    using namespace boost::asio;
    if (iIpType == MNI_IPV4)
    {
        ip::address_v4::bytes_type bytes;
        memmove(bytes.data(), byteIp, 4);
        epAddr.address(ip::address(ip::address_v4(bytes)));
    }
    else if (iIpType == MNI_IPV6)
    {
        ip::address_v6::bytes_type bytes;
        memmove(bytes.data(), byteIp, 16);
        epAddr.address(ip::address(ip::address_v6(bytes)));
    }
    else
    {
        return false;
    }
    epAddr.port(usPort);
    return true;
}

string CMthNetEndpoint::ToString()
{
    if (iIpType == MNI_IPV4)
    {
        return (strIp + string(":") + to_string(usPort));
    }
    else if (iIpType == MNI_IPV6)
    {
        return (string("[") + strIp + string("]:") + to_string(usPort));
    }
    else
    {
        return "";
    }
}


//----------------------------------------------------------------------------------------
CNetDataQueue::~CNetDataQueue()
{
    while(!qData.empty())
    {
        CMthNetPackData *pPack = qData.front();
        if (pPack)
        {
            delete pPack;
        }
        qData.pop();
    }
    mapNetPort.clear();
}

bool CNetDataQueue::SetData(CMthNetPackData*& data, uint32 ui32Timeout)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!PrtSetData(lock, data, ui32Timeout))
    {
        return false;
    }
    DoSetNetCount(data->ui64NetId, NULL);
    return true;
}

bool CNetDataQueue::GetData(CMthNetPackData*& data, uint32 ui32Timeout)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!PrtGetData(lock, data, ui32Timeout))
    {
        return false;
    }
    DoGetNetCount(data->ui64NetId);
    return true;
}

bool CNetDataQueue::PushPacket(CMthNetPackData* pData, uint32& nPackCount, uint32 ui32Timeout)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!PrtSetData(lock, pData, ui32Timeout))
    {
        return false;
    }
    nPackCount = DoSetNetCount(pData->ui64NetId, NULL);
    return true;
}

bool CNetDataQueue::PushPacket(CMthNetPackData* pData, bool& fLimitRecv, uint32 ui32Timeout)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!PrtSetData(lock, pData, ui32Timeout))
    {
        return false;
    }
    fLimitRecv = false;
    DoSetNetCount(pData->ui64NetId, &fLimitRecv);
    return true;
}

bool CNetDataQueue::FetchPacket(CMthNetPackData*& pData, uint32& nPackCount, uint32 ui32Timeout)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!PrtGetData(lock, pData, ui32Timeout))
    {
        return false;
    }
    nPackCount = DoGetNetCount(pData->ui64NetId);
    return true;
}

uint32 CNetDataQueue::QueryNetPackCountByNetId(uint64 nNetId)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    map<uint64, pair<uint32,bool>>::iterator it;
    it = mapNetPort.find(nNetId);
    if (it != mapNetPort.end())
    {
        return it->second.first;
    }
    return 0;
}

bool CNetDataQueue::FrontPacket(CMthNetPackData& tPacket)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!qData.empty())
    {
        tPacket = *(qData.front());
        return true;
    }
    return false;
}

void CNetDataQueue::PopPacket()
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    if (!qData.empty())
    {
        uint64 nNetId = qData.front()->ui64NetId;
        qData.pop();
        DoGetNetCount(nNetId);
    }
}

bool CNetDataQueue::SetLowTrigger(uint32 nLowCount, CallLowTriggerBackFunc fnTrigCallbackIn)
{
    boost::unique_lock<boost::mutex> lock(lockQueue);
    nTriggerLowCount = nLowCount;
    fnTrigCallback = fnTrigCallbackIn;
    return true;
}

inline uint32 CNetDataQueue::DoSetNetCount(uint64 nNetId, bool *pIfLimitRecv)
{
    uint32 nPackCount = 0;
    map<uint64, pair<uint32,bool>>::iterator it = mapNetPort.find(nNetId);
    if (it != mapNetPort.end())
    {
        nPackCount = ++it->second.first;
        if (pIfLimitRecv && fnTrigCallback && nTriggerLowCount > 0 && it->second.first >= nTriggerLowCount)
        {
            *pIfLimitRecv = true;
            it->second.second = true;
        }
    }
    else
    {
        mapNetPort.insert(make_pair(nNetId,make_pair(1,false)));
        nPackCount = 1;
    }
    return nPackCount;
}

inline uint32 CNetDataQueue::DoGetNetCount(uint64 nNetId)
{
    uint32 nCount = 0;
    bool fNeedTrig = false;
    map<uint64, pair<uint32,bool>>::iterator it = mapNetPort.find(nNetId);
    if (it != mapNetPort.end())
    {
        if (it->second.first <= 1)
        {
            nCount = 0;
            fNeedTrig = it->second.second;
            mapNetPort.erase(it);
            it = mapNetPort.end();
        }
        else
        {
            nCount = --it->second.first;
            if (it->second.second)
            {
                fNeedTrig = true;
            }
        }
    }
    if (fNeedTrig && fnTrigCallback && nTriggerLowCount > 0 && nCount < nTriggerLowCount)
    {
        fnTrigCallback(nNetId);
        if (it != mapNetPort.end())
        {
            it->second.second = false;
        }
    }
    return nCount;
}

}  // namespace network

