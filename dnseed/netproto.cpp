// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netproto.h"

#include "blockhead/util.h"

using boost::asio::ip::tcp;

namespace dnseed
{

//-----------------------------------------------------------------------------------------------
CProtoDataBuf::CProtoDataBuf()
  : fPacketVerifyIntegrity(false)
{
}

CProtoDataBuf::CProtoDataBuf(uint32 ui32MsgMagic, int nChannel, int nCommand, CBlockheadBufStream& ssPayload)
  : fPacketVerifyIntegrity(false)
{
    AllocPacketBuf(ui32MsgMagic, nChannel, nCommand, ssPayload);
}

CProtoDataBuf::~CProtoDataBuf()
{
}

uint8 CProtoDataBuf::GetChannel() const
{
    if (pDataBuf == NULL || ui32DataLen < NMS_MESSAGE_HEADER_SIZE)
    {
        return 0;
    }
    return (((PNMS_MSG_HEAD)pDataBuf)->nType >> 6);
}

uint8 CProtoDataBuf::GetCommand() const
{
    if (pDataBuf == NULL || ui32DataLen < NMS_MESSAGE_HEADER_SIZE)
    {
        return 0;
    }
    return (((PNMS_MSG_HEAD)pDataBuf)->nType & 0x3F);
}

unsigned char* CProtoDataBuf::GetPayload(uint32& ui32PayloadLen)
{
    if (!fPacketVerifyIntegrity)
    {
        return NULL;
    }
    ui32PayloadLen = ((PNMS_MSG_HEAD)pDataBuf)->nPayloadSize;
    return (unsigned char*)pDataBuf + NMS_MESSAGE_HEADER_SIZE;
}

bool CProtoDataBuf::GetPayload(CBlockheadBufStream& ssPayload)
{
    if (!fPacketVerifyIntegrity)
    {
        return false;
    }
    ssPayload.Write(pDataBuf + NMS_MESSAGE_HEADER_SIZE, ((PNMS_MSG_HEAD)pDataBuf)->nPayloadSize);
    return true;
}

bool CProtoDataBuf::CheckPacketIntegrity()
{
    if (pDataBuf == NULL || ui32DataLen < NMS_MESSAGE_HEADER_SIZE)
    {
        return false;
    }
    if (((PNMS_MSG_HEAD)pDataBuf)->nPayloadSize >= NMS_MESSAGE_PAYLOAD_MAX_SIZE)
    {
        clear();
        return false;
    }
    if (ui32DataLen < NMS_MESSAGE_HEADER_SIZE + ((PNMS_MSG_HEAD)pDataBuf)->nPayloadSize)
    {
        return false;
    }
    return true;
}

bool CProtoDataBuf::VerifyPacket(uint32 ui32MsgMagic)
{
    PNMS_MSG_HEAD pMsgHead = (PNMS_MSG_HEAD)pDataBuf;
    unsigned char* pPayloadPos = (unsigned char*)pDataBuf + NMS_MESSAGE_HEADER_SIZE;

    if (pMsgHead->nMagic != ui32MsgMagic)
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Magic error.");
        ErasePacket();
        return false;
    }

    if (bigbang::crypto::crc24q((unsigned char*)pDataBuf, 13) != (*(uint32*)&pDataBuf[13] & 0xFFFFFF))
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Header check sum error.");
        ErasePacket();
        return false;
    }

    if (pMsgHead->nPayloadChecksum != bigbang::crypto::CryptoHash(pPayloadPos, pMsgHead->nPayloadSize).Get32())
    {
        blockhead::StdError(__PRETTY_FUNCTION__, "Payload check sum error.");
        ErasePacket();
        return false;
    }
    fPacketVerifyIntegrity = true;

    return true;
}

void CProtoDataBuf::ErasePacket()
{
    if (CheckPacketIntegrity())
    {
        erase(NMS_MESSAGE_HEADER_SIZE + ((PNMS_MSG_HEAD)pDataBuf)->nPayloadSize);
    }
    fPacketVerifyIntegrity = false;
}

bool CProtoDataBuf::AllocPacketBuf(uint32 ui32MsgMagic, int nChannel, int nCommand, CBlockheadBufStream& ssPayload)
{
    uint32 nPacketSize = NMS_MESSAGE_HEADER_SIZE + ssPayload.GetSize();
    reserve(nPacketSize);

    PNMS_MSG_HEAD pMsgHead = (PNMS_MSG_HEAD)pDataBuf;
    pMsgHead->nMagic = ui32MsgMagic;
    pMsgHead->nType = GetMessageType(nChannel, nCommand);
    pMsgHead->nPayloadSize = ssPayload.GetSize();
    pMsgHead->nPayloadChecksum = bigbang::crypto::CryptoHash(ssPayload.GetData(), ssPayload.GetSize()).Get32();
    pMsgHead->nHeaderChecksum = GetHeaderChecksum((unsigned char*)pMsgHead);

    if (ssPayload.GetSize())
    {
        memcpy(pDataBuf + NMS_MESSAGE_HEADER_SIZE, ssPayload.GetData(), ssPayload.GetSize());
    }
    ui32DataLen = nPacketSize;

    return VerifyPacket(ui32MsgMagic);
}

///////////////////////////////
// CEndpoint

static const unsigned char epipv4[CEndpoint::BINSIZE] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0xff,
    0xff,
};
CEndpoint::CEndpoint()
  : CBinary((char*)ss, BINSIZE)
{
    memmove(ss, epipv4, BINSIZE);
}

CEndpoint::CEndpoint(const tcp::endpoint& ep)
  : CBinary((char*)ss, BINSIZE)
{
    SetEndpoint(ep);
}

CEndpoint::CEndpoint(const CEndpoint& other)
  : CBinary((char*)ss, BINSIZE)
{
    other.CopyTo(ss);
}

void CEndpoint::SetEndpoint(const tcp::endpoint& ep)
{
    memmove(ss, epipv4, BINSIZE);
    if (ep.address().is_v4())
    {
        //TODO
        memmove(&ss[12], ep.address().to_v4().to_bytes().data(), 4);
    }
    else
    {
        //TODO
        memmove(&ss[0], ep.address().to_v6().to_bytes().data(), 16);
    }
    ss[16] = ep.port() >> 8;
    ss[17] = ep.port() & 0xFF;
}

void CEndpoint::GetEndpoint(tcp::endpoint& ep)
{
    using namespace boost::asio;

    if (memcmp(ss, epipv4, 12) == 0)
    {
        ip::address_v4::bytes_type bytes;
        //TODO
        memmove(bytes.data(), &ss[12], 4);
        ep.address(ip::address(ip::address_v4(bytes)));
    }
    else
    {
        ip::address_v6::bytes_type bytes;
        //TODO
        memmove(bytes.data(), &ss[0], 16);
        ep.address(ip::address(ip::address_v6(bytes)));
    }
    ep.port((((unsigned short)ss[16]) << 8) + ss[17]);
}

void CEndpoint::CopyTo(unsigned char* ssTo) const
{
    memmove(ssTo, ss, BINSIZE);
}

bool CEndpoint::IsRoutable()
{
    if (memcmp(epipv4, ss, 12) == 0)
    {
        // IPV4
        //RFC1918
        if (ss[12] == 10 || (ss[12] == 192 && ss[13] == 168)
            || (ss[12] == 172 && ss[13] >= 16 && ss[13] <= 31))
        {
            return false;
        }

        //RFC3927
        if (ss[12] == 169 && ss[13] == 254)
        {
            return false;
        }

        //Local
        if (ss[12] == 127 || ss[12] == 0)
        {
            return false;
        }
    }
    else
    {
        // IPV6
        //RFC4862
        const unsigned char pchRFC4862[] = { 0xFE, 0x80, 0, 0, 0, 0, 0, 0 };
        if (memcmp(ss, pchRFC4862, sizeof(pchRFC4862)) == 0)
        {
            return false;
        }

        //RFC4193
        if ((ss[0] & 0xFE) == 0xFC)
        {
            return false;
        }

        //RFC4843
        if (ss[0] == 0x20 && ss[1] == 0x01 && ss[2] == 0x00 && (ss[3] & 0xF0) == 0x10)
        {
            return false;
        }

        //Local
        const unsigned char pchLocal[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
        if (memcmp(ss, pchLocal, sizeof(pchLocal)) == 0)
        {
            return false;
        }
    }
    return true;
}

///////////////////////////////
// CAddress

void CAddress::SetAddress(uint64 nServiceIn, const tcp::endpoint& ep)
{
    nService = nServiceIn;
    ssEndpoint.SetEndpoint(ep);
}

} // namespace dnseed
