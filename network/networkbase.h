// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __NETWORK_NETBASE_H
#define __NETWORK_NETBASE_H

#include <boost/asio.hpp>

#include "blockhead/type.h"
#include "nbase/mthbase.h"

namespace network
{

using namespace std;
using namespace nbase;
using boost::asio::ip::tcp;

class CMthNetIp
{
public:
    enum
    {
        MNI_UNKNOWN = 0,
        MNI_IPV4 = 4,
        MNI_IPV6 = 6
    };

    CMthNetIp()
      : iIpType(MNI_UNKNOWN) {}
    CMthNetIp(const unsigned char* pIpByte, const int iIpTypeIn);
    CMthNetIp(const CMthNetIp& na);
    ~CMthNetIp() {}

    bool SetIp(const char* pIpIn);
    bool SetIp(const string& strIpIn);
    string& GetIp();
    int GetIpType() const
    {
        return iIpType;
    }

    CMthNetIp& operator=(CMthNetIp& na)
    {
        memcpy(byteIp, na.byteIp, 16);
        iIpType = na.iIpType;
        strIp = na.strIp;
        return *this;
    }
    friend bool operator==(const CMthNetIp& a, const CMthNetIp& b)
    {
        return (memcmp(a.byteIp, a.byteIp, 16) == 0 && a.iIpType == b.iIpType);
    }
    friend bool operator!=(const CMthNetIp& a, const CMthNetIp& b)
    {
        return !(memcmp(a.byteIp, a.byteIp, 16) == 0 && a.iIpType == b.iIpType);
    }

    static bool ToByteIpaddr(const char* pIpStr, unsigned char* pIpByte, int* pIpType);                           // ip type: 4: ipv4, 6: ipv6
    static bool ToStrIpaddr(const unsigned char* pIpByte, const int iIpType, char* pIpStrBuf, int iIpStrBufSize); // ip type: 4: ipv4, 6: ipv6
    static bool SplitHostPort(const char* pAddr, char* pHost, int iHostBufSize, unsigned short* pPort);

protected:
    unsigned char byteIp[16]; // in network byte order
    int iIpType;              // ip type: 4: ipv4, 6: ipv6
    string strIp;
};

class CMthNetEndpoint : public CMthNetIp
{
public:
    CMthNetEndpoint()
      : usPort(0) {}
    CMthNetEndpoint(const CMthNetEndpoint& ep)
      : usPort(ep.usPort), CMthNetIp(ep.byteIp, ep.iIpType) {}

    bool SetAddrPort(const char* pIpIn, const uint16 nPortIn);
    bool SetAddrPort(string& strIpIn, uint16 nPortIn);
    bool SetAddrPort(tcp::endpoint& ep);
    bool SetAddrPort(const char* pIpPort);
    bool SetAddrPort(string& strIpPort);

    void SetPort(const uint16 nPortIn)
    {
        usPort = nPortIn;
    }
    uint16 GetPort() const
    {
        return usPort;
    }
    string ToString();

    bool SetEndpoint(const tcp::endpoint& epAddr);
    bool GetEndpoint(tcp::endpoint& epAddr);

    CMthNetEndpoint& operator=(CMthNetEndpoint& ep) //override
    {
        memcpy(byteIp, ep.byteIp, sizeof(byteIp));
        iIpType = ep.iIpType;
        strIp = ep.strIp;
        usPort = ep.usPort;
        return *this;
    }
    friend bool operator==(const CMthNetEndpoint& a, const CMthNetEndpoint& b)
    {
        return (memcmp(a.byteIp, a.byteIp, 16) == 0 && a.iIpType == b.iIpType && a.usPort == b.usPort);
    }
    friend bool operator!=(const CMthNetEndpoint& a, const CMthNetEndpoint& b)
    {
        return !(memcmp(a.byteIp, a.byteIp, 16) == 0 && a.iIpType == b.iIpType && a.usPort == b.usPort);
    }

protected:
    uint16 usPort;
};

typedef enum _E_NET_MSG_TYPE
{
    NET_MSG_TYPE_SETUP,
    NET_MSG_TYPE_DATA,
    NET_MSG_TYPE_CLOSE,
    NET_MSG_TYPE_COMPLETE_NOTIFY
} E_NET_MSG_TYPE;

typedef enum _E_DISCONNECT_CAUSE
{
    NET_DIS_CAUSE_UNKNOWN,
    NET_DIS_CAUSE_PEER_CLOSE,
    NET_DIS_CAUSE_LOCAL_CLOSE,
    NET_DIS_CAUSE_CONNECT_SUCCESS,
    NET_DIS_CAUSE_CONNECT_FAIL
} E_DISCONNECT_CAUSE;

class CMthNetPackData : public CMthDataBuf
{
public:
    CMthNetPackData()
      : ui64NetId(0) {}
    CMthNetPackData(uint64 nNetIdIn, E_NET_MSG_TYPE eMsgTypeIn, E_DISCONNECT_CAUSE eCauseIn,
                    CMthNetEndpoint& naPeer, CMthNetEndpoint& naLocal, char* p, uint32 n)
      : ui64NetId(nNetIdIn), eMsgType(eMsgTypeIn), eDisCause(eCauseIn),
        epPeerAddr(naPeer), epLocalAddr(naLocal), CMthDataBuf(p, n)
    {
    }
    CMthNetPackData(uint64 nNetIdIn, E_NET_MSG_TYPE eMsgTypeIn, E_DISCONNECT_CAUSE eCauseIn,
                    CMthNetEndpoint& naPeer, CMthNetEndpoint& naLocal)
      : ui64NetId(nNetIdIn), eMsgType(eMsgTypeIn), eDisCause(eCauseIn),
        epPeerAddr(naPeer), epLocalAddr(naLocal)
    {
    }

public:
    uint64 ui64NetId;
    E_NET_MSG_TYPE eMsgType;
    E_DISCONNECT_CAUSE eDisCause;
    CMthNetEndpoint epPeerAddr;
    CMthNetEndpoint epLocalAddr;
};

class CNetDataQueue : public CMthQueue<CMthNetPackData*>
{
public:
    typedef boost::function<void(uint64)> CallLowTriggerBackFunc;

    CNetDataQueue()
      : nTriggerLowCount(0), fnTrigCallback(NULL) {}
    CNetDataQueue(uint32 ui32QueueSize)
      : CMthQueue<CMthNetPackData*>(ui32QueueSize), nTriggerLowCount(0), fnTrigCallback(NULL) {}
    ~CNetDataQueue();

    /* ui32Timeout is milliseconds */
    bool SetData(CMthNetPackData*& data, uint32 ui32Timeout = 0) override;
    bool GetData(CMthNetPackData*& data, uint32 ui32Timeout = 0) override;

    bool PushPacket(CMthNetPackData* pData, uint32& nPackCount, uint32 ui32Timeout = 0);
    bool PushPacket(CMthNetPackData* pData, bool& fLimitRecv, uint32 ui32Timeout = 0);
    bool FetchPacket(CMthNetPackData*& pData, uint32& nPackCount, uint32 ui32Timeout = 0);

    uint32 QueryNetPackCountByNetId(uint64 nNetId);
    bool FrontPacket(CMthNetPackData& tPacket);
    void PopPacket();
    bool SetLowTrigger(uint32 nLowCount, CallLowTriggerBackFunc fnTrigCallbackIn);

private:
    uint32 DoSetNetCount(uint64 nNetId, bool* pIfLimitRecv);
    uint32 DoGetNetCount(uint64 nNetId);

protected:
    map<uint64, pair<uint32, bool>> mapNetPort;
    uint32 nTriggerLowCount;
    CallLowTriggerBackFunc fnTrigCallback;
};

} // namespace network

#endif // __NETWORK_NETBASE_H
