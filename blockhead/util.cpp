// Copyright (c) 2019 The Bigbang developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "util.h"
#include <map>

using namespace std;

namespace blockhead
{
bool STD_DEBUG = false;
bool fDaemonRun = false;
boost::mutex lockLog;

} // namespace blockhead


