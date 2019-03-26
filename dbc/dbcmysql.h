// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DBC_DBCMYSQL_H
#define __DBC_DBCMYSQL_H

#include <iostream>
#include <vector>
#include "dbcacc.h"
#include <mysql.h>
#include <boost/thread/thread.hpp>

namespace dbc
{

using namespace std;

class CDbcMysqlDbConnect;

//--------------------------------------------------------------------------------
class CDbcMysqlSelect : virtual public CDbcSelect
{
public:
    CDbcMysqlSelect(CDbcMysqlDbConnect* pDbConnIn, const string& strSqlIn);
    ~CDbcMysqlSelect();

    bool Query();

    void Release() override;
    unsigned int GetFieldCount() override;
    bool GetFieldInfo(unsigned int uiFieldIndex, string& sFieldName, unsigned int& uiFieldType, unsigned int& uiFieldLen) override;
    bool GetFieldName(unsigned int uiFieldIndex, string& sFieldName) override;
    bool GetFieldType(unsigned int uiFieldIndex, unsigned int& uiFieldType) override;
    bool GetFieldLen(unsigned int uiFieldIndex, unsigned int& uiFieldLen) override;
    bool MoveNext() override;

    unsigned char* GetFieldBuf(unsigned int uiFieldIndex, unsigned int& uiValueLen) override;
    bool GetField(unsigned int uiFieldIndex, char& cValue) override;
    bool GetField(unsigned int uiFieldIndex, unsigned char& ucValue) override;
    bool GetField(unsigned int uiFieldIndex, short& sValue) override;
    bool GetField(unsigned int uiFieldIndex, unsigned short& usValue) override;
    bool GetField(unsigned int uiFieldIndex, int& iValue) override;
    bool GetField(unsigned int uiFieldIndex, unsigned int& uiValue) override;
    bool GetField(unsigned int uiFieldIndex, long& lValue) override;
    bool GetField(unsigned int uiFieldIndex, unsigned long& ulValue) override;
    bool GetField(unsigned int uiFieldIndex, long long& llValue) override;
    bool GetField(unsigned int uiFieldIndex, unsigned long long& ullValue) override;
    bool GetField(unsigned int uiFieldIndex, float& fValue) override;
    bool GetField(unsigned int uiFieldIndex, double& dValue) override;
    bool GetField(unsigned int uiFieldIndex, string& strOut) override;
    bool GetField(unsigned int uiFieldIndex, vector<unsigned char>& vFieldValue) override;

private:
    CDbcMysqlDbConnect* pDbConn;
    string strSql;

    MYSQL_RES *pMysqlRes;
    MYSQL_ROW pMysqlRow;
    unsigned int uiFieldCount;
    unsigned long *pFieldLenTable;
};


class CDbcMysqlDbConnect : virtual public CDbcDbConnect
{
    friend class CDbcMysqlSelect;
public:
    CDbcMysqlDbConnect(CDbcConfig& tDbcCfgIn);
    ~CDbcMysqlDbConnect();

    bool ConnectDb() override;
    void DisconnectDb() override;
    void Timer();
    void SetCommitCount(int iCount) override;
    int GetCommitCount() override;
    bool ExecuteStaticSql(const string& strSql) override;
    CDbcSelect* Query(const string& strSql) override;
    void Release() override;

    string ToEscString(const string& str) override;
    string ToEscString(const void* pBinary,size_t nBytes) override;
    string ToEscString(const std::vector<unsigned char>& vch) override
    {
        return ToEscString(&vch[0],vch.size());
    }

private:
    bool PrConnectDb();
    void PrDisconnectDb();
    void PrCommit();

    CDbcConfig tDbcCfg;
    int iCommitCount;

    MYSQL tMysqlConn;
    bool fIsConnect;
    bool fIsAutoCommit;
    int iStaticExeCount;
    time_t tmPrevCommitTime;

    boost::mutex lockConn;
};

} // namespace dbc

#endif //__DBC_DBCMYSQL_H

