#------------------------------------------------------------------------------
# CMake file for Bigbang
#
# Copyright (c) 2019 The Bigbang developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#------------------------------------------------------------------------------

set(sources
	uint256.h
	crc24q.cpp crc24q.h
	crypto.cpp crypto.h
)

add_library(crypto ${sources})

include_directories(../ ${sodium_INCLUDE_DIR})

target_link_libraries(crypto
	Boost::system
	Boost::filesystem
	Boost::program_options
	Boost::thread
	Boost::date_time
	Boost::regex
    OpenSSL::SSL
    OpenSSL::Crypto
	${sodium_LIBRARY_RELEASE}
	blockhead
)
