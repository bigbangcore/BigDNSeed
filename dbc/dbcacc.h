// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DBC_DBCACC_H
#define __DBC_DBCACC_H

#include <iostream>
#include <vector>

namespace dbc
{

using namespace std;

//--------------------------------------------------------------------------------
enum {
    DBC_DBTYPE_MYSQL = 1,
    DBC_DBTYPE_ORACLE = 2,
    DBC_DBTYPE_SQLSERVER = 3
};

class CDbcConfig
{
public:
    CDbcConfig() {}
    CDbcConfig(CDbcConfig &tCfg) : iDbType(tCfg.iDbType),sDbIp(tCfg.sDbIp),
        usDbPort(tCfg.usDbPort),sDbName(tCfg.sDbName),sDbUser(tCfg.sDbUser),sDbPwd(tCfg.sDbPwd) {}
    ~CDbcConfig() {}

public:
    int iDbType;
    string sDbIp;
    unsigned short usDbPort;
    string sDbName;
    string sDbUser;
    string sDbPwd;
};


//-----------------------------------------------------------------------------
class CDbcSelect
{
public:
    virtual ~CDbcSelect() {}

    virtual void Release() = 0;
    virtual unsigned int GetFieldCount() = 0;
    virtual bool GetFieldInfo(unsigned int uiFieldIndex, string& sFieldName, unsigned int& uiFieldType, unsigned int& uiFieldLen) = 0;
    virtual bool GetFieldName(unsigned int uiFieldIndex, string& sFieldName) = 0;
    virtual bool GetFieldType(unsigned int uiFieldIndex, unsigned int& uiFieldType) = 0;
    virtual bool GetFieldLen(unsigned int uiFieldIndex, unsigned int& uiFieldLen) = 0;
    virtual bool MoveNext() = 0;

    virtual unsigned char* GetFieldBuf(unsigned int uiFieldIndex, unsigned int& uiValueLen) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, char& cValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, unsigned char& ucValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, short& sValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, unsigned short& usValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, int& iValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, unsigned int& uiValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, long& lValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, unsigned long& ulValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, long long& llValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, unsigned long long& ullValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, float& fValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, double& dValue) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, string& strOut) = 0;
    virtual bool GetField(unsigned int uiFieldIndex, vector<unsigned char>& vFieldValue) = 0;
};


//-----------------------------------------------------------------------------
class CDbcDbConnect
{
public:
    virtual ~CDbcDbConnect() {}

    virtual bool ConnectDb() = 0;
    virtual void DisconnectDb() = 0;
    virtual void Timer() = 0;
    virtual void SetCommitCount(int iCount) = 0;
    virtual int GetCommitCount() = 0;
    virtual bool ExecuteStaticSql(const string& strSql) = 0;
    virtual CDbcSelect* Query(const string& strSql) = 0;
    virtual void Release() = 0;

    virtual string ToEscString(const string& str) = 0;
    virtual string ToEscString(const void* pBinary,size_t nBytes) = 0;
    virtual string ToEscString(const std::vector<unsigned char>& vch) = 0;

    static CDbcDbConnect* DbcCreateDbConnObj(CDbcConfig& tDbcCfg);
};


} // namespace dbc

#endif //__DBC_DBCACC_H

