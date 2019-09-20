
add_library(redisdb STATIC
    redisclient.cc
    redisdatabase.cc
)

target_link_libraries(redisdb
    runos_core
)

set_target_properties(redisdb PROPERTIES
    LANGUAGE CXX
    )

