
set(sources
        version.h
        addrpool.cpp addrpool.h
        config.cpp config.h
        dbstorage.cpp dbstorage.h
        dispatcher.cpp dispatcher.h
        entry.cpp entry.h
        netmsgwork.cpp netmsgwork.h
        netpeer.cpp netpeer.h
        netproto.cpp netproto.h
        )

add_library (dnseed ${sources})

target_link_libraries(dnseed
        blockhead
        network
        nbase
        dbc
        crypto
        )
