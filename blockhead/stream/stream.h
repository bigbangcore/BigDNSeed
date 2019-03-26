// Copyright (c) 2019 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef  BLOCKHEAD_STREAM_H
#define  BLOCKHEAD_STREAM_H

#include "blockhead/type.h"
#include "blockhead/stream/circular.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <boost/type_traits.hpp>
#include <boost/asio.hpp>

namespace blockhead
{

enum
{
    BLOCKHEAD_STREAM_SAVE,
    BLOCKHEAD_STREAM_LOAD
};

typedef const boost::true_type  BasicType;
typedef const boost::false_type ObjectType;

typedef const boost::integral_constant<int, BLOCKHEAD_STREAM_SAVE> SaveType;
typedef const boost::integral_constant<int, BLOCKHEAD_STREAM_LOAD> LoadType;

// Base class of serializable stream
class CBlockheadStream
{
public:
    CBlockheadStream(std::streambuf* sb) : ios(sb) {}

    virtual std::size_t GetSize() {return 0;}
    
    std::iostream& GetIOStream()
    {
        return ios;
    }

    CBlockheadStream& Write(const char *s,std::size_t n)
    {
        ios.write(s,n);
        return (*this);
    }

    CBlockheadStream& Read(char *s,std::size_t n)
    {
        ios.read(s,n);
        return (*this);
    }

    template <typename T,typename O>
    CBlockheadStream& Serialize(T& t,O &opt)
    {
        return Serialize(t,boost::is_fundamental<T>(),opt);
    }

    template <typename T>
    CBlockheadStream& operator<< (const T& t)
    {
        return Serialize(const_cast<T&>(t),boost::is_fundamental<T>(),SaveType());
    }

    template <typename T>
    CBlockheadStream& operator>> (T& t)
    {
        return Serialize(t,boost::is_fundamental<T>(),LoadType());
    }

    template <typename T>
    std::size_t GetSerializeSize(const T& t)
    {
        std::size_t serSize = 0;
        Serialize(const_cast<T&>(t),boost::is_fundamental<T>(),serSize);
        return serSize;
    }

protected:
    template <typename T>
    CBlockheadStream& Serialize(const T& t,BasicType&,SaveType&)
    {
        return Write((const char *)&t,sizeof(t));
    }

    template <typename T>
    CBlockheadStream& Serialize(T& t,BasicType&,LoadType&)
    {
        return Read((char *)&t,sizeof(t));
    }

    template <typename T>
    CBlockheadStream& Serialize(const T& t,BasicType&,std::size_t& serSize)
    {
        serSize += sizeof(t);
        return (*this);
    }

    template <typename T,typename O>
    CBlockheadStream& Serialize(T& t,ObjectType&,O& opt)
    {
        t.BlockheadSerialize(*this,opt);
        return (*this);
    }

    /* std::string */
    CBlockheadStream& Serialize(std::string& t,ObjectType&,SaveType&);
    CBlockheadStream& Serialize(std::string& t,ObjectType&,LoadType&);
    CBlockheadStream& Serialize(std::string& t,ObjectType&,std::size_t& serSize);

    /* std::vector */
    template<typename T, typename A>
    CBlockheadStream& Serialize(std::vector<T, A>& t,ObjectType&,SaveType&);
    template<typename T, typename A>
    CBlockheadStream& Serialize(std::vector<T, A>& t,ObjectType&,LoadType&);
    template<typename T, typename A>
    CBlockheadStream& Serialize(std::vector<T, A>& t,ObjectType&,std::size_t& serSize);

    /* std::pair */
    template<typename P1, typename P2,typename O>
    CBlockheadStream& Serialize(std::pair<P1, P2>& t,ObjectType&,O& o);
protected:
    std::iostream ios;
};

// Autosize buffer stream
class CBlockheadBufStream : public boost::asio::streambuf, public CBlockheadStream
{
public:
    CBlockheadBufStream() : CBlockheadStream(this) {}

    void Clear()
    {
        ios.clear();
        consume(size());
    }

    char *GetData() const
    {
        return gptr();
    }

    std::size_t GetSize()
    {
        return size();
    }

    void HexToString(std::string& strHex)
    {
        const char hexc[17] = "0123456789abcdef";
        char strByte[4] = "00 ";
        strHex.clear();
        strHex.reserve(size() * 3);
        char *p = gptr();
        while(p != pptr())
        {
            int c = (int)((unsigned char *)p++)[0];
            strByte[0] = hexc[c >> 4];
            strByte[1] = hexc[c & 15];
            strHex.append(strByte);
        }
    }

    void Dump()
    {
        std::cout << "CBlockheadStream Dump : " << size() << std::endl << std::hex;
        char *p = gptr();
        int n = 0;
        std::cout.setf(std::ios_base::hex);
        while(p != pptr())
        {
            std::cout << std::setfill('0') << std::setw(2) << (int)((unsigned char *)p++)[0] << " ";
            if (++n == 32)
            {
                std::cout << std::endl;
                n = 0;
            }
        }
        std::cout.unsetf(std::ios::hex);
        if (n != 0)
        {
            std::cout << std::endl;
        }
    }

    friend CBlockheadStream& operator<< (CBlockheadStream& s,CBlockheadBufStream& ssAppend)
    {
        return s.Write(ssAppend.gptr(),ssAppend.GetSize());
    }

    friend CBlockheadStream& operator>> (CBlockheadStream& s,CBlockheadBufStream& ssSink)
    {
        std::size_t len = s.GetSize();
        ssSink.prepare(len);
        s.Read(ssSink.pptr(),len);
        ssSink.commit(len);
        return s;
    }
};

// Circular buffer stream
class CBlockheadCircularStream : public circularbuf, public CBlockheadStream
{
public:
    CBlockheadCircularStream(std::size_t nMaxSize)
    : circularbuf(nMaxSize) ,CBlockheadStream(this) {}

    void Clear()
    {
        ios.clear();
        consume(size());
    }

    std::size_t GetSize()
    {
        return size();
    }

    std::size_t GetBufFreeSpace() const
    {
        return freespace();
    }

    std::size_t GetWritePos() const
    {
        return putpos();
    }

    bool Seek(std::size_t nPos)
    {
        ios.clear();
        return (seekpos(nPos) >= 0);
    }

    void Rewind()
    {
        ios.clear();
        seekoff(0,std::ios_base::beg);
    }

    void Consume(std::size_t nSize)
    {
        consume(nSize);
    }

    void Dump()
    {
        std::cout << "CBlockheadStream Dump : " << size() << std::endl << std::hex;
        seekoff(0,std::ios_base::beg);

        int n = 0;
        std::cout.setf(std::ios_base::hex);
        while(in_avail() != 0)
        {
            int c = sbumpc();

            std::cout << std::setfill('0') << std::setw(2) << c << " ";
            if (++n == 32)
            {
                std::cout << std::endl;
                n = 0;
            }
        }
        std::cout.unsetf(std::ios::hex);
        if (n != 0)
        {
            std::cout << std::endl;
        }
    }
};

// File stream with compatible serialization mothed
class CBlockheadFileStream : public std::filebuf, public CBlockheadStream
{

public:
    CBlockheadFileStream(const char* filename) : CBlockheadStream(this)
    {
        open(filename,std::ios_base::in | std::ios_base::out
                      | std::ios_base::binary);
    }
    ~CBlockheadFileStream()
    {
        close();
    }

    bool IsValid() const
    {
        return is_open();
    }

    bool IsEOF() const
    {
        return ios.eof();
    }

    std::size_t GetSize()
    {
        std::streampos cur = seekoff(0,std::ios_base::cur);
        std::size_t size = (std::size_t)(seekoff(0,std::ios_base::end) - cur);
        seekpos(cur);
        return size;
    }

    std::size_t GetCurPos()
    {
        return (std::size_t)seekoff(0,std::ios_base::cur);
    }

    void Seek(std::size_t pos)
    {
        seekpos((std::streampos)pos);
    }

    void SeekToBegin()
    {
        seekoff(0,std::ios_base::beg);
    }

    void SeekToEnd()
    {
        seekoff(0,std::ios_base::end);
    }

    void Sync()
    {
        sync();
    }
};

// R/W compact size
//  size <  253        -- 1 byte
//  size <= USHRT_MAX  -- 3 bytes  (253 + 2 bytes)
//  size <= UINT_MAX   -- 5 bytes  (254 + 4 bytes)
//  size >  UINT_MAX   -- 9 bytes  (255 + 8 bytes)
class CVarInt
{
    friend class CBlockheadStream;
public:
    CVarInt() : nValue(0) {}
    CVarInt(uint64 ValueIn) : nValue(ValueIn) {}
protected:
    void BlockheadSerialize(CBlockheadStream& s,SaveType&);
    void BlockheadSerialize(CBlockheadStream& s,LoadType&);
    void BlockheadSerialize(CBlockheadStream& s,std::size_t& serSize);
public:
    uint64 nValue;
};

// Binary memory 
class CBinary
{
    friend class CBlockheadStream;
public:
    CBinary(char *pBufferIn,std::size_t nLengthIn) : pBuffer(pBufferIn),nLength(nLengthIn) {}
    friend bool operator==(const CBinary& a,const CBinary& b)
    {
        return (a.nLength == b.nLength
                && (a.nLength == 0 || std::memcmp(a.pBuffer,b.pBuffer,a.nLength) == 0));
    }
    friend bool operator!=(const CBinary& a,const CBinary& b)
    {
        return (!(a == b));
    }
protected:
    void BlockheadSerialize(CBlockheadStream& s,SaveType&);
    void BlockheadSerialize(CBlockheadStream& s,LoadType&);
    void BlockheadSerialize(CBlockheadStream& s,std::size_t& serSize);

protected:
    char *pBuffer;
    std::size_t nLength;
};

/* CBlockheadStream vector serialize impl */
template<typename T, typename A>
CBlockheadStream& CBlockheadStream::Serialize(std::vector<T, A>& t,ObjectType&,SaveType&)
{
    *this << CVarInt(t.size());
    if (boost::is_fundamental<T>::value)
    {
        Write((char *)&t[0],sizeof(T) * t.size());
    }
    else
    {
        for (uint64 i = 0;i < t.size();i++)
        {
            *this << t[i];
        }
    }
    return (*this);
}

template<typename T, typename A>
CBlockheadStream& CBlockheadStream::Serialize(std::vector<T, A>& t,ObjectType&,LoadType&)
{
    CVarInt var;
    *this >> var;
    t.resize(var.nValue);
    if (boost::is_fundamental<T>::value)
    {
        Read((char *)&t[0],sizeof(T) * t.size());
    }
    else
    {
        for (uint64 i = 0;i < var.nValue;i++)
        {
            *this >> t[i];
        }
    }
    return (*this);
}

template<typename T, typename A>
CBlockheadStream& CBlockheadStream::Serialize(std::vector<T, A>& t,ObjectType&,std::size_t& serSize)
{
    CVarInt var(t.size());
    serSize += GetSerializeSize(var);
    if (boost::is_fundamental<T>::value)
    {
        serSize += sizeof(T) * t.size();
    }
    else
    {
        for (uint64 i = 0;i < t.size();i++)
        {
            serSize += GetSerializeSize(t[i]);
        }
    }
    return (*this);
}

template<typename P1, typename P2,typename O>
CBlockheadStream& CBlockheadStream::Serialize(std::pair<P1, P2>& t,ObjectType&,O& o)
{
    return Serialize(t.first,o).Serialize(t.second,o);
}

template<typename T>
std::size_t GetSerializeSize(const T& obj)
{
    CBlockheadBufStream ss;
    return ss.GetSerializeSize(obj);
}

} // namespace blockhead

#endif //BLOCKHEAD_STREAM_H

