
include_directories(./)

set(sources
        type.h util.cpp util.h nettime.h
        stream/circular.cpp stream/circular.h 
        stream/datastream.h
        stream/stream.cpp stream/stream.h 
        )

add_library (blockhead ${sources})

