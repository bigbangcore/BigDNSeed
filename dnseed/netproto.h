// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_NETPROTO_H
#define __DNSEED_NETPROTO_H

#include <iostream>

#include "blockhead/stream/stream.h"
#include "blockhead/type.h"
#include "crypto/crc24q.h"
#include "crypto/crypto.h"
#include "nbase/mthbase.h"

namespace dnseed
{

using namespace std;
using namespace blockhead;
using namespace nbase;

//---------------------------------------------------------------------------------
// basic config
#define MAINNET_MAGICNUM 0x3b54beae
#define TESTNET_MAGICNUM 0xa006c295

#define NMS_MESSAGE_HEADER_SIZE 16
#define NMS_MESSAGE_PAYLOAD_MAX_SIZE 0x400000

//---------------------------------------------------------------------------------
enum
{
    NODE_NETWORK = (1 << 0),
    NODE_DELEGATED = (1 << 1),
};

enum
{
    BBPROTO_CHN_NETWORK = 0,
    BBPROTO_CHN_DELEGATE = 1,
    BBPROTO_CHN_DATA = 2,
    BBPROTO_CHN_USER = 3,
};

enum
{
    BBPROTO_CMD_HELLO = 1,
    BBPROTO_CMD_HELLO_ACK = 2,
    BBPROTO_CMD_GETADDRESS = 3,
    BBPROTO_CMD_ADDRESS = 4,
    BBPROTO_CMD_PING = 5,
    BBPROTO_CMD_PONG = 6,
};

//---------------------------------------------------------------------------------
#pragma pack(1)

typedef struct _NMS_MSG_HEAD
{
    uint32 nMagic;
    uint8 nType;
    uint32 nPayloadSize;
    uint32 nPayloadChecksum;
    uint32 nHeaderChecksum;

} NMS_MSG_HEAD, *PNMS_MSG_HEAD;

#pragma pack()

//---------------------------------------------------------------------------------
class CProtoDataBuf : public CMthDataBuf
{
public:
    CProtoDataBuf();
    CProtoDataBuf(uint32 ui32MsgMagic, int nChannel, int nCommand, CBlockheadBufStream& ssPayload);
    ~CProtoDataBuf();

public:
    uint8 GetChannel() const;
    uint8 GetCommand() const;

    static uint8 GetMessageType(int nChannel, int nCommand)
    {
        return ((nChannel << 6) | (nCommand & 0x3F));
    }
    static uint32 GetHeaderChecksum(unsigned char* pBuf)
    {
        return bigbang::crypto::crc24q(pBuf, 13);
    }

    unsigned char* GetPayload(uint32& ui32PayloadLen);
    bool GetPayload(CBlockheadBufStream& ssPayload);
    bool CheckPacketIntegrity();
    bool VerifyPacket(uint32 ui32MsgMagic);
    void ErasePacket();
    bool AllocPacketBuf(uint32 ui32MsgMagic, int nChannel, int nCommand,
                        CBlockheadBufStream& ssPayload);

private:
    bool fPacketVerifyIntegrity;
};

class CEndpoint : public blockhead::CBinary
{
public:
    enum
    {
        BINSIZE = 18
    };
    CEndpoint();
    CEndpoint(const boost::asio::ip::tcp::endpoint& ep);
    CEndpoint(const CEndpoint& other);
    void SetEndpoint(const boost::asio::ip::tcp::endpoint& ep);
    void GetEndpoint(boost::asio::ip::tcp::endpoint& ep);
    void CopyTo(unsigned char* ssTo) const;
    bool IsRoutable();
    const CEndpoint& operator=(const CEndpoint& other)
    {
        other.CopyTo(ss);
        return (*this);
    }

protected:
    unsigned char ss[BINSIZE];
};

class CAddress
{
    friend class blockhead::CBlockheadStream;

public:
    CAddress() {}
    CAddress(uint64 nServiceIn, const boost::asio::ip::tcp::endpoint& ep)
      : nService(nServiceIn), ssEndpoint(ep) {}
    CAddress(uint64 nServiceIn, const CEndpoint& ssEndpointIn)
      : nService(nServiceIn), ssEndpoint(ssEndpointIn) {}

    void SetAddress(uint64 nServiceIn, const boost::asio::ip::tcp::endpoint& ep);

protected:
    template <typename O>
    void BlockheadSerialize(blockhead::CBlockheadStream& s, O& opt)
    {
        s.Serialize(nService, opt);
        s.Serialize(ssEndpoint, opt);
    }

public:
    uint64 nService;
    CEndpoint ssEndpoint;
};

} // namespace dnseed

#endif //__DNSEED_NETPROTO_H
