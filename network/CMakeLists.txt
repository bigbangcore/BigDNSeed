
set(sources
        networkbase.cpp networkbase.h
        networkservice.cpp networkservice.h
        networkthread.cpp networkthread.h
        tcpconnect.cpp tcpconnect.h
        )

add_library (network ${sources})

target_link_libraries(network
        blockhead
        nbase
        )
