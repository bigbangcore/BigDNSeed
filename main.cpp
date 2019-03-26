// Copyright (c) 2019 The BigDNSeed developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
#include "blockhead/type.h"
#include "blockhead/util.h"
#include "dnseed/entry.h"

using namespace std;
using namespace dnseed;
using namespace blockhead;


void BbShutdown()
{
    CBbEntry::GetInstance().Stop();
}

int main(int argc,char **argv)
{
    CBbEntry& bbEntry = CBbEntry::GetInstance();
    try
    {
        if (bbEntry.Initialize(argc,argv))
        {
            blockhead::StdLog("MAIN", "Initialize success.");
            bbEntry.Run();
        }
    }
    catch (std::exception& e)
    {
        StdError(__PRETTY_FUNCTION__, e.what());
    }
    catch (...)
    {
        StdError(__PRETTY_FUNCTION__, "unknown");
    }
    
    bbEntry.Stop();

    blockhead::StdLog("MAIN", "Program quit.");

    return 0;
}

