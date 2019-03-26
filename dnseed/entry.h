// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __DNSEED_ENTRY_H
#define __DNSEED_ENTRY_H

#include <string>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include "dispatcher.h"
#include "config.h"
#include "version.h"

namespace dnseed
{

class CBbEntry
{
public:
    static CBbEntry& GetInstance();

public:
    ~CBbEntry();
    bool Initialize(int argc,char *argv[]);

    bool TryLockFile(const std::string& strLockFile);
    bool Run();
    void Stop();

protected:
    void HandleSignal(const boost::system::error_code& error,int signal_number);
    void HandleTimer(const boost::system::error_code &err);

    boost::filesystem::path GetDefaultDataDir();

    void PurgeStorage();
    bool RunInBackground(const boost::filesystem::path& pathData);
    void ExitBackground(const boost::filesystem::path& pathData);

protected:
    boost::asio::io_service ioService;
    boost::asio::signal_set ipcSignals;
    boost::interprocess::file_lock lockFile;

    CDnseedConfig tDnseedCfg;
    CDispatcher *pDisp;

    boost::asio::deadline_timer *pTimerStat;

private:
    CBbEntry();

    void Uninitialize();
};

} // namespace dnseed

#endif //__DNSEED_ENTRY_H

