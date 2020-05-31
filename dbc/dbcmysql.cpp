// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbcmysql.h"

#include <iostream>

#include "string.h"


using namespace std;

namespace dbc
{

//-----------------------------------------------------------------------------
class CMysqlLib
{
public:
    CMysqlLib()
    {
        mysql_library_init(0, NULL, NULL);
    }
    ~CMysqlLib()
    {
        mysql_library_end();
    }
};

static CMysqlLib __mysqlLib;

//-----------------------------------------------------------------------------
CDbcMysqlSelect::CDbcMysqlSelect(CDbcMysqlDbConnect* pDbConnIn, const string& strSqlIn)
  : pDbConn(pDbConnIn), strSql(strSqlIn), pMysqlRes(NULL), pMysqlRow(NULL), uiFieldCount(0), pFieldLenTable(NULL)
{
}

CDbcMysqlSelect::~CDbcMysqlSelect()
{
    if (pMysqlRes)
    {
        mysql_free_result(pMysqlRes);
        pMysqlRes = NULL;
    }
}

bool CDbcMysqlSelect::Query()
{
    if (pDbConn == NULL || strSql.empty())
    {
        return false;
    }

    if (!pDbConn->PrConnectDb())
    {
        return false;
    }

    if (mysql_real_query(&pDbConn->tMysqlConn, strSql.c_str(), strSql.size()))
    {
        fprintf(stderr, "Failed to execute mysql_real_query: Error: %s.\n",
                mysql_error(&pDbConn->tMysqlConn));
        return false;
    }

    if (pMysqlRes)
    {
        mysql_free_result(pMysqlRes);
        pMysqlRes = NULL;
    }
    pMysqlRow = NULL;
    pFieldLenTable = NULL;

    pMysqlRes = mysql_store_result(&pDbConn->tMysqlConn);
    if (pMysqlRes == NULL)
    {
        fprintf(stderr, "Failed to execute mysql_store_result: Error: %s.\n",
                mysql_error(&pDbConn->tMysqlConn));
        return false;
    }

    uiFieldCount = mysql_num_fields(pMysqlRes);

    return true;
}

//------------------------------------------------------------------------------------
void CDbcMysqlSelect::Release()
{
    delete this;
}

unsigned int CDbcMysqlSelect::GetFieldCount()
{
    if (pMysqlRes == NULL)
    {
        return 0;
    }
    return uiFieldCount;
}

bool CDbcMysqlSelect::GetFieldInfo(unsigned int uiFieldIndex, string& sFieldName, unsigned int& uiFieldType, unsigned int& uiFieldLen)
{
    if (pMysqlRes == NULL || uiFieldIndex >= uiFieldCount)
    {
        return false;
    }
    MYSQL_FIELD* pField = mysql_fetch_field_direct(pMysqlRes, uiFieldIndex);
    if (pField == NULL)
    {
        return false;
    }
    sFieldName = string(pField->name);
    uiFieldType = pField->type;
    uiFieldLen = pField->length;
    return true;
}

bool CDbcMysqlSelect::GetFieldName(unsigned int uiFieldIndex, string& sFieldName)
{
    if (pMysqlRes == NULL || uiFieldIndex >= uiFieldCount)
    {
        return false;
    }
    MYSQL_FIELD* pField = mysql_fetch_field_direct(pMysqlRes, uiFieldIndex);
    if (pField == NULL)
    {
        return false;
    }
    sFieldName = string(pField->name);
    return true;
}

bool CDbcMysqlSelect::GetFieldType(unsigned int uiFieldIndex, unsigned int& uiFieldType)
{
    if (pMysqlRes == NULL || uiFieldIndex >= uiFieldCount)
    {
        return false;
    }
    MYSQL_FIELD* pField = mysql_fetch_field_direct(pMysqlRes, uiFieldIndex);
    if (pField == NULL)
    {
        return false;
    }
    uiFieldType = pField->type;
    return true;
}

bool CDbcMysqlSelect::GetFieldLen(unsigned int uiFieldIndex, unsigned int& uiFieldLen)
{
    if (pMysqlRes == NULL || uiFieldIndex >= uiFieldCount)
    {
        return false;
    }
    MYSQL_FIELD* pField = mysql_fetch_field_direct(pMysqlRes, uiFieldIndex);
    if (pField == NULL)
    {
        return false;
    }
    uiFieldLen = pField->length;
    return true;
}

bool CDbcMysqlSelect::MoveNext()
{
    if (pMysqlRes == NULL)
    {
        return false;
    }
    pMysqlRow = mysql_fetch_row(pMysqlRes);
    pFieldLenTable = mysql_fetch_lengths(pMysqlRes);
    return true;
}

//----------------------------------------------------------------------------------------------------
unsigned char* CDbcMysqlSelect::GetFieldBuf(unsigned int uiFieldIndex, unsigned int& uiValueLen)
{
    if (pMysqlRow == NULL || pFieldLenTable == NULL || uiFieldIndex >= uiFieldCount)
    {
        return NULL;
    }
    uiValueLen = (unsigned int)pFieldLenTable[uiFieldIndex];
    return (unsigned char*)(pMysqlRow[uiFieldIndex]);
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, char& cValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    cValue = (char)atoi(pMysqlRow[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, unsigned char& ucValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    ucValue = (unsigned char)atoi(pMysqlRow[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, short& sValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    sValue = (short)atoi(pMysqlRow[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, unsigned short& usValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    usValue = (unsigned short)atoi(pMysqlRow[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, int& iValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    iValue = atoi(pMysqlRow[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, unsigned int& uiValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    uiValue = (unsigned int)(std::stoul(pMysqlRow[uiFieldIndex], nullptr, 10));
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, long& lValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    lValue = std::stol(pMysqlRow[uiFieldIndex], nullptr, 10);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, unsigned long& ulValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    ulValue = std::stoul(pMysqlRow[uiFieldIndex], nullptr, 10);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, long long& llValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    llValue = std::stoll(pMysqlRow[uiFieldIndex], nullptr, 10);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, unsigned long long& ullValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    ullValue = std::stoull(pMysqlRow[uiFieldIndex], nullptr, 10);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, float& fValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    fValue = std::stof(pMysqlRow[uiFieldIndex], nullptr);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, double& dValue)
{
    if (pMysqlRow == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    dValue = std::stod(pMysqlRow[uiFieldIndex], nullptr);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, string& strOut)
{
    if (pMysqlRow == NULL || pFieldLenTable == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    strOut = string(pMysqlRow[uiFieldIndex], pFieldLenTable[uiFieldIndex]);
    return true;
}

bool CDbcMysqlSelect::GetField(unsigned int uiFieldIndex, vector<unsigned char>& vFieldValue)
{
    if (pMysqlRow == NULL || pFieldLenTable == NULL || uiFieldIndex >= uiFieldCount || pMysqlRow[uiFieldIndex] == NULL)
    {
        return false;
    }
    try
    {
        vFieldValue.assign(pMysqlRow[uiFieldIndex], pMysqlRow[uiFieldIndex] + pFieldLenTable[uiFieldIndex]);
    }
    catch (std::exception& e)
    {
        fprintf(stderr, "Failed to GetVector: Error: %s.\n", e.what());
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------
CDbcMysqlDbConnect::CDbcMysqlDbConnect(CDbcConfig& tDbcCfgIn)
  : tDbcCfg(tDbcCfgIn), fIsConnect(false), fIsAutoCommit(true), iStaticExeCount(0), iCommitCount(0)
{
    mysql_init(&tMysqlConn);
}

CDbcMysqlDbConnect::~CDbcMysqlDbConnect()
{
    PrDisconnectDb();
}

bool CDbcMysqlDbConnect::PrConnectDb()
{
    bool fRet = false;
    if (fIsConnect)
    {
        return true;
    }

    fRet = (mysql_real_connect(
                &tMysqlConn,
                tDbcCfg.sDbIp.c_str(),
                tDbcCfg.sDbUser.c_str(),
                tDbcCfg.sDbPwd.c_str(),
                tDbcCfg.sDbName.c_str(),
                tDbcCfg.usDbPort,
                NULL, 0)
            != NULL);
    if (fRet)
    {
        fIsConnect = true;

        char cReConnect = 1;
        mysql_options(&tMysqlConn, MYSQL_OPT_RECONNECT, &cReConnect);
        if (iCommitCount > 1)
        {
            mysql_autocommit(&tMysqlConn, 0);
            fIsAutoCommit = false;
        }
        mysql_set_character_set(&tMysqlConn, "utf8");
    }
    else
    {
        fprintf(stderr, "Failed to connect to database: Error: %s.\n",
                mysql_error(&tMysqlConn));
    }

    return fRet;
}

void CDbcMysqlDbConnect::PrDisconnectDb()
{
    if (fIsConnect)
    {
        if (!fIsAutoCommit)
        {
            if (iStaticExeCount > 0)
            {
                mysql_commit(&tMysqlConn);
                iStaticExeCount = 0;
            }
            mysql_autocommit(&tMysqlConn, 1);
            fIsAutoCommit = true;
        }
        mysql_close(&tMysqlConn);
        fIsConnect = false;
    }
}

void CDbcMysqlDbConnect::PrCommit()
{
    time_t tmCurTime = time(NULL);
    if (iStaticExeCount >= iCommitCount || tmCurTime - tmPrevCommitTime >= 1 || tmCurTime < tmPrevCommitTime)
    {
        if (iStaticExeCount > 0)
        {
            if (!fIsAutoCommit)
            {
                mysql_commit(&tMysqlConn);
            }
            iStaticExeCount = 0;
        }
        tmPrevCommitTime = tmCurTime;
    }
}

//------------------------------------------------------------------------------
bool CDbcMysqlDbConnect::ConnectDb()
{
    boost::unique_lock<boost::mutex> lock(lockConn);
    return PrConnectDb();
}

void CDbcMysqlDbConnect::DisconnectDb()
{
    boost::unique_lock<boost::mutex> lock(lockConn);
    PrDisconnectDb();
}

void CDbcMysqlDbConnect::Timer()
{
    boost::unique_lock<boost::mutex> lock(lockConn);
    if (mysql_ping(&tMysqlConn) != 0)
    {
        fprintf(stderr, "Failed to mysql_ping: Error: %s.\n",
                mysql_error(&tMysqlConn));
        return;
    }
    PrCommit();
}

void CDbcMysqlDbConnect::SetCommitCount(int iCount)
{
    boost::unique_lock<boost::mutex> lock(lockConn);

    iCommitCount = iCount;
    if (fIsConnect)
    {
        if (iCommitCount > 1)
        {
            if (fIsAutoCommit)
            {
                mysql_autocommit(&tMysqlConn, 0);
                fIsAutoCommit = false;
            }
        }
        else
        {
            if (!fIsAutoCommit)
            {
                if (iStaticExeCount > 0)
                {
                    mysql_commit(&tMysqlConn);
                    iStaticExeCount = 0;
                }
                mysql_autocommit(&tMysqlConn, 1);
                fIsAutoCommit = true;
            }
        }
    }
}

int CDbcMysqlDbConnect::GetCommitCount()
{
    return iCommitCount;
}

bool CDbcMysqlDbConnect::ExecuteStaticSql(const string& strSql)
{
    boost::unique_lock<boost::mutex> lock(lockConn);
    if (strSql.empty())
    {
        return false;
    }
    if (!PrConnectDb())
    {
        return false;
    }
    if (mysql_real_query(&tMysqlConn, strSql.c_str(), strSql.size()))
    {
        fprintf(stderr, "Failed to execute mysql_real_query: Error: %s, Sql: %s.\n",
                mysql_error(&tMysqlConn), strSql.c_str());
        return false;
    }
    iStaticExeCount++;
    PrCommit();
    return true;
}

CDbcSelect* CDbcMysqlDbConnect::Query(const string& strSql)
{
    boost::unique_lock<boost::mutex> lock(lockConn);
    if (strSql.empty())
    {
        return NULL;
    }
    if (!PrConnectDb())
    {
        return NULL;
    }
    CDbcMysqlSelect* pSelect = new CDbcMysqlSelect(this, strSql);
    if (!pSelect->Query())
    {
        delete pSelect;
        return NULL;
    }
    return (CDbcSelect*)pSelect;
}

void CDbcMysqlDbConnect::Release()
{
    delete this;
}

string CDbcMysqlDbConnect::ToEscString(const string& str)
{
    char s[str.size() * 2 + 1];
    return string(s, mysql_real_escape_string(&tMysqlConn, s, str.c_str(), str.size()));
}

string CDbcMysqlDbConnect::ToEscString(const void* pBinary, size_t nBytes)
{
    char s[nBytes * 2 + 1];
    return string(s, mysql_real_escape_string(&tMysqlConn, s, (const char*)pBinary, nBytes));
}

} // namespace dbc
