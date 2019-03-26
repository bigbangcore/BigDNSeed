// Copyright (c) 2019 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef  BLOCKHEAD_DATASTREAM_H
#define  BLOCKHEAD_DATASTREAM_H

#include <exception>
#include <cstring>
#include <vector>
#include <map>
#include <boost/type_traits.hpp>

namespace blockhead
{

class CBlockheadODataStream
{
#define BEGIN(a)            ((unsigned char*)&(a))
#define END(a)              ((unsigned char*)&((&(a))[1]))
public:
    CBlockheadODataStream(std::vector<unsigned char>& vchIn) : vch(vchIn) {}
    void Push(const void* p,std::size_t size)
    {
        vch.insert(vch.end(),(const unsigned char*)p,(const unsigned char*)p + size);
    }
     
    template <typename T>
    void Push(const T& data,const boost::true_type&)
    {
        vch.insert(vch.end(),BEGIN(data),END(data));
    }
    template <typename T>
    void Push(const T& data,const boost::false_type&)
    {
        data.ToDataStream(*this);
    }
    
    template <typename T>
    CBlockheadODataStream& operator<< (const T& data)
    {
        Push(data,boost::is_fundamental<T>());
        return (*this);
    }
    template<typename T, typename A>
    CBlockheadODataStream& operator<< (const std::vector<T, A>& data)
    {
        unsigned int size = data.size();
        vch.insert(vch.end(),BEGIN(size),END(size));
        if (size > 0)
        {
            if (boost::is_fundamental<T>::value)
            {
                vch.insert(vch.end(),BEGIN(data[0]),END(data[size - 1]));
            }
            else
            {
                for (std::size_t i = 0;i < data.size();i++)
                {
                    (*this) << data[i];
                }
            }
        }
        return (*this);
    }
    template<typename K, typename V, typename C, typename A>
    CBlockheadODataStream& operator<< (const std::map<K, V, C, A>& data)
    {
        unsigned int size = data.size();
        vch.insert(vch.end(),BEGIN(size),END(size));
        for (typename std::map<K, V, C, A>::const_iterator it = data.begin();it != data.end();++it)
        {
            (*this) << (*it).first << (*it).second;
        }
        return (*this);
    }
protected:
    std::vector<unsigned char>& vch;
    std::size_t nPosition;
};

class CBlockheadIDataStream
{
public:
    CBlockheadIDataStream(const std::vector<unsigned char>& vchIn) : vch(vchIn),nPosition(0) {}
    std::size_t GetSize()
    {
        return (vch.size() - nPosition);
    }
    void Pop(void* p,std::size_t size)
    {
        if (nPosition + size > vch.size())
        {
            throw std::range_error("out of range");
        }
        std::memmove(p,&vch[nPosition],size);
        nPosition += size;
    }
    template <typename T>
    void Pop(T& data,const boost::true_type&)
    {
        if (nPosition + sizeof(T) > vch.size())
        {
            throw std::range_error("out of range");
        }
        data = *((T*)&vch[nPosition]);
        nPosition += sizeof(T);
    }
    template <typename T>
    void Pop(T& data,const boost::false_type&)
    {
        data.FromDataStream(*this);
    }
    template <typename T>
    CBlockheadIDataStream& operator>> (T& data)
    {
        Pop(data,boost::is_fundamental<T>());
        return (*this);
    }
    template<typename T, typename A>
    CBlockheadIDataStream& operator>> (std::vector<T, A>& data)
    {
        unsigned int size;
        *this >> size;
        if (boost::is_fundamental<T>::value)
        {
            data.assign((T*)&vch[nPosition],(T*)&vch[nPosition] + size);
            nPosition += size * sizeof(T); 
        }
        else
        {
            data.resize(size);
            for (unsigned int i = 0;i < size;i++)
            {
                *this >> data[i];
            }
        }
        return (*this);
    }
    template<typename K, typename V, typename C, typename A>
    CBlockheadIDataStream& operator>> (std::map<K, V, C, A>& data)
    {
        unsigned int size;
        *this >> size;
        for (std::size_t i = 0;i < size;i++)
        {
            K k;
            V v;
            *this >> k >> v;
            if (!data.insert(std::make_pair(k,v)).second)
            {
                throw std::runtime_error("data conflict");
            }
        }
        return (*this);
    }
protected:
    const std::vector<unsigned char>& vch;
    std::size_t nPosition;
};

} // namespace blockhead

#endif //BLOCKHEAD_DATASTREAM_H

