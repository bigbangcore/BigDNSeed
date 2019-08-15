#------------------------------------------------------------------------------
# CMake file for BigDNSeed
#
# Copyright (c) 2019 The BigDNSeed developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#------------------------------------------------------------------------------

project(bigdnseed LANGUAGES CXX)

cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}/cmake" CACHE PATH "The path to the cmake directory")
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
        ./
        )

add_subdirectory(dnseed)
add_subdirectory(nbase)
add_subdirectory(network)
add_subdirectory(dbc)
add_subdirectory(blockhead)
add_subdirectory(crypto)

set(sources
        main.cpp
        )

add_executable(bigdnseed ${sources})

target_link_libraries(bigdnseed
        blockhead
        network
        nbase
        dnseed
        dbc
        crypto
        )

target_link_libraries(bigdnseed
        Boost::system
        Boost::filesystem
        Boost::program_options
        Boost::thread
        Boost::date_time
        Boost::regex
        Boost::serialization
        OpenSSL::SSL
        OpenSSL::Crypto
        )

INSTALL(TARGETS bigdnseed RUNTIME DESTINATION bin)

