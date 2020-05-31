// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dbcacc.h"

#include "dbcmysql.h"

using namespace std;

namespace dbc
{

//--------------------------------------------------------------------------------
CDbcDbConnect* CDbcDbConnect::DbcCreateDbConnObj(CDbcConfig& tDbcCfg)
{
    switch (tDbcCfg.iDbType)
    {
    case DBC_DBTYPE_MYSQL:
        return (CDbcDbConnect*)new CDbcMysqlDbConnect(tDbcCfg);
    default:
        break;
    }
    return NULL;
}

} // namespace dbc
