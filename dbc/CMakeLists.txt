
include_directories(
        /usr/include
        /usr/include/mysql
        ./
        )

set(sources
        dbcmysql.cpp dbcmysql.h
        dbcacc.cpp dbcacc.h
        )

add_library (dbc ${sources})

target_link_libraries(dbc
        ${MYSQL_LIB}
        )
