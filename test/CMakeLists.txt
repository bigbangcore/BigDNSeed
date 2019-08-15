#------------------------------------------------------------------------------
# CMake file for BigDNSeed_test
#
# Copyright (c) 2019 The BigDNSeed_test developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#------------------------------------------------------------------------------

project(bigdnseed_test LANGUAGES CXX)

cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/../cmake" CACHE PATH "The path to the cmake directory")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_DIR})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
elseif(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
endif()

# packages
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.58.0 REQUIRED COMPONENTS
    program_options
    filesystem
    system
    thread
    date_time
    regex
    unit_test_framework
    serialization
)
find_package(OpenSSL 1.0.0 REQUIRED) 
find_package(MySQL 5.7.20 REQUIRED)
find_package(sodium 1.0.8 REQUIRED)

if(${USE_SSL_110} MATCHES "TRUE")
    add_definitions(-DUSE_SSL_110)
endif(${USE_SSL_110} MATCHES "TRUE")

include_directories(
        /usr/include
        ${sodium_INCLUDE_DIR}
        ./
        ../
        )

set(sources
        main_test.cpp
        stresstest.cpp stresstest.h
        ../blockhead/type.h ../blockhead/util.cpp ../blockhead/util.h ../blockhead/nettime.h
        ../blockhead/stream/circular.cpp ../blockhead/stream/circular.h 
        ../blockhead/stream/datastream.h
        ../blockhead/stream/stream.cpp ../blockhead/stream/stream.h 
        ../crypto/crc24q.cpp ../crypto/crc24q.h
        ../crypto/crypto.cpp ../crypto/crypto.h ../crypto/uint256.h
        ../network/networkthread.cpp ../network/networkthread.h
        ../network/tcpconnect.cpp ../network/tcpconnect.h
        ../network/networkservice.cpp ../network/networkservice.h
        ../network/networkbase.cpp ../network/networkbase.h
        ../nbase/mthbase.cpp ../nbase/mthbase.h
        ../dbc/dbcacc.cpp ../dbc/dbcacc.h
        ../dbc/dbcmysql.cpp ../dbc/dbcmysql.h
        ../dnseed/addrpool.cpp ../dnseed/addrpool.h
        ../dnseed/config.cpp ../dnseed/config.h
        ../dnseed/dbstorage.cpp ../dnseed/dbstorage.h
        ../dnseed/dispatcher.cpp ../dnseed/dispatcher.h
        ../dnseed/entry.cpp ../dnseed/entry.h
        ../dnseed/netmsgwork.cpp ../dnseed/netmsgwork.h
        ../dnseed/netpeer.cpp ../dnseed/netpeer.h
        ../dnseed/netproto.cpp ../dnseed/netproto.h
        )

add_executable(bigdnseed_test ${sources})

target_link_libraries(bigdnseed_test
        Boost::system
        Boost::filesystem
        Boost::program_options
        Boost::thread
        Boost::date_time
        Boost::regex
        Boost::serialization
        OpenSSL::SSL
        OpenSSL::Crypto
        ${MYSQL_LIB}
	    ${sodium_LIBRARY_RELEASE}
        )

INSTALL(TARGETS bigdnseed_test RUNTIME DESTINATION bin)

