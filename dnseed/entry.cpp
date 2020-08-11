// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "entry.h"

#include <boost/bind.hpp>
#include <signal.h>
#include <stdio.h>

using namespace std;
using namespace blockhead;
using namespace boost::filesystem;

namespace dnseed
{

//////////////////////////////
// CBbEntry

CBbEntry& CBbEntry::GetInstance()
{
    static CBbEntry entry;
    return entry;
}

//------------------------------------------------------------------------------
CBbEntry::CBbEntry()
  : ioService(), ipcSignals(ioService), pDisp(NULL), pTimerStat(NULL)
{
    ipcSignals.add(SIGINT);
    ipcSignals.add(SIGTERM);
#if defined(SIGQUIT)
    ipcSignals.add(SIGQUIT);
#endif // defined(SIGQUIT)
#if defined(SIGHUP)
    ipcSignals.add(SIGHUP);
#endif // defined(SIGHUP)

    ipcSignals.async_wait(boost::bind(&CBbEntry::HandleSignal, this, _1, _2));
}

CBbEntry::~CBbEntry()
{
    Stop();
}

bool CBbEntry::Initialize(int argc, char* argv[])
{
    if (!tDnseedCfg.ReadConfig(argc, argv, GetDefaultDataDir(), "bigdnseed.conf"))
    {
        cerr << "Failed read config.\n";
        return false;
    }

    // stop
    if (tDnseedCfg.fStop)
    {
        ExitBackground(tDnseedCfg.pathData);
        return false;
    }

    // help
    if (tDnseedCfg.fHelp)
    {
        tDnseedCfg.Help();
        return false;
    }

    // version
    if (tDnseedCfg.fVersion)
    {
        cout << "BigDNSeed version is v" << BB_VERSION_STR << endl;
        return false;
    }

    // purge
    if (tDnseedCfg.fPurge)
    {
        PurgeStorage();
        return false;
    }

    // list config if in debug mode
    if (tDnseedCfg.fDebug)
    {
        tDnseedCfg.ListConfig();
    }

    // path
    path& pathData = tDnseedCfg.pathData;
    if (!exists(pathData))
    {
        if (!create_directories(pathData))
        {
            cerr << "Failed create directory : " << pathData << "\n";
            return false;
        }
    }

    if (!is_directory(pathData))
    {
        cerr << "Failed to access data directory : " << pathData << "\n";
        return false;
    }

    // daemon
    if (tDnseedCfg.fDaemon)
    {
        if (!RunInBackground(pathData))
        {
            return false;
        }
        cout << "bigdnseed server starting\n";
    }

    if (!TryLockFile((tDnseedCfg.pathData / ".lock").string()))
    {
        cerr << "Cannot obtain a lock on data directory " << tDnseedCfg.pathData << "\n"
             << "bigdnseed is probably already running.\n";
        return false;
    }

    if (!InitLog(pathData, tDnseedCfg.fDebug, tDnseedCfg.fDaemon, tDnseedCfg.nLogFileSize, tDnseedCfg.nLogHistorySize))
    {
        cerr << "Failed to open log file : " << (pathData / "bigbang.log") << "\n";
        return false;
    }

    //----------------------------------------------------------------------
    pDisp = new CDispatcher();
    if (!pDisp->Initialize(&tDnseedCfg))
    {
        cerr << "Error: Initialize fail.\n";
        Uninitialize();
        return false;
    }

    if (!pDisp->StartService())
    {
        cerr << "Error: StartService fail.\n";
        Uninitialize();
        return false;
    }

    return true;
}

void CBbEntry::Uninitialize()
{
    if (pDisp)
    {
        pDisp->StopService();
        pDisp->Deinitialize();

        delete pDisp;
        pDisp = NULL;
    }
}

bool CBbEntry::TryLockFile(const string& strLockFile)
{
    FILE* fp = fopen(strLockFile.c_str(), "a");
    if (fp)
        fclose(fp);
    lockFile = boost::interprocess::file_lock(strLockFile.c_str());
    return lockFile.try_lock();
}

bool CBbEntry::Run()
{
    try
    {
        pTimerStat = new boost::asio::deadline_timer(ioService, boost::posix_time::seconds(1));
        pTimerStat->async_wait(boost::bind(&CBbEntry::HandleTimer, this, _1));

        ioService.run();
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void CBbEntry::Stop()
{
    Uninitialize();
    ioService.stop();

    if (pTimerStat)
    {
        pTimerStat->cancel();
        delete pTimerStat;
        pTimerStat = NULL;
    }
}

void CBbEntry::HandleSignal(const boost::system::error_code& error, int signal_number)
{
    if (signal_number == SIGINT || signal_number == SIGTERM)
    {
        Stop();
    }
}

void CBbEntry::HandleTimer(const boost::system::error_code& err)
{
    if (!err)
    {
        boost::posix_time::seconds needTime(1);
        pTimerStat->expires_at(pTimerStat->expires_at() + needTime);
        pTimerStat->async_wait(boost::bind(&CBbEntry::HandleTimer, this, _1));

        if (pDisp)
        {
            pDisp->Timer();
        }

        if (tDnseedCfg.fDaemon)
        {
            FILE* file = fopen((tDnseedCfg.pathData / "bigdnseed.pid").string().c_str(), "r");
            if (file == NULL)
            {
                Stop();
            }
            else
            {
                fclose(file);
            }
        }
    }
}

path CBbEntry::GetDefaultDataDir()
{
    // Windows: C:\Documents and Settings\username\Local Settings\Application Data\bigdnseed
    // Mac: ~/Library/Application Support/bigdnseed
    // Unix: ~/.bigdnseed

#ifdef WIN32
    // Windows
    char pszPath[MAX_PATH] = "";
    if (SHGetSpecialFolderPathA(NULL, pszPath, CSIDL_LOCAL_APPDATA, true))
    {
        return path(pszPath) / "bigdnseed";
    }
    return path("C:\\bigdnseed");
#else
    path pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
    {
        pathRet = path("/");
    }
    else
    {
        pathRet = path(pszHome);
    }
#ifdef __APPLE__
    // Mac
    pathRet /= "Library/Application Support";
    create_directory(pathRet);
    return pathRet / "bigdnseed";
#else
    // Unix
    return pathRet / ".bigdnseed";
#endif
#endif
}

void CBbEntry::PurgeStorage()
{
    path& pathData = tDnseedCfg.pathData;

    if (!TryLockFile((pathData / ".lock").string()))
    {
        cerr << "Cannot obtain a lock on data directory " << pathData << "\n"
             << "BigDNSeed is probably already running.\n";
        return;
    }

    CDbStorage tDbAcc(&tDnseedCfg.tDbCfg, NULL);
    if (!tDbAcc.PurgeData())
    {
        cerr << "Purge data fail.\n";
        return;
    }
    cout << "Purge data success.\n";
}

bool CBbEntry::RunInBackground(const path& pathData)
{
#ifndef WIN32
    // Daemonize
    ioService.notify_fork(boost::asio::io_service::fork_prepare);

    pid_t pid = fork();
    if (pid < 0)
    {
        cerr << "Error: fork() returned " << pid << " errno " << errno << "\n";
        return false;
    }
    if (pid > 0)
    {
        FILE* file = fopen((pathData / "bigdnseed.pid").string().c_str(), "w");
        if (file)
        {
            fprintf(file, "%d\n", pid);
            fclose(file);
        }
        exit(0);
    }

    pid_t sid = setsid();
    if (sid < 0)
    {
        cerr << "Error: setsid) returned " << sid << " errno " << errno << "\n";
        return false;
    }
    ioService.notify_fork(boost::asio::io_service::fork_child);
    return true;

#else
    HWND hwnd = GetForegroundWindow();
    cout << "daemon running, window will run in background" << endl;
    system("pause");
    ShowWindow(hwnd, SW_HIDE);
    return true;
#endif
    return false;
}

void CBbEntry::ExitBackground(const path& pathData)
{
#ifndef WIN32
    boost::filesystem::remove(pathData / "bigdnseed.pid");
#endif
}

} // namespace dnseed
