
set(sources
        mthbase.cpp mthbase.h
        )

add_library (nbase ${sources})

target_link_libraries(nbase
        blockhead
        )

