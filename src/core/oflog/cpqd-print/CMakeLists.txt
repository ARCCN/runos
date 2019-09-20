if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions(
        -Wno-int-conversion
        -Wno-shift-negative-value
        -Wno-incompatible-pointer-types-discards-qualifiers
        -Wno-implicit-function-declaration
    )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions(
        -Wno-discarded-qualifiers
        -Wno-implicit-function-declaration
    )
endif()

add_library(cpqd_print STATIC
    dynamic-string.c
    hmap.c
    ofpbuf.c
    timeval.c
    util.c
    fatal-signal.c
    random.c
    vlog.c
    open_memstream.c
    oxm-match.c
    ofl-print.c
    ofl-actions.c
    ofl-actions-print.c
    ofl-actions-unpack.c
    ofl-structs.c
    ofl-structs-match.c
    ofl-structs-print.c
    ofl-structs-unpack.c
    ofl-messages.c
    ofl-messages-print.c
    ofl-messages-unpack.c
)

set_target_properties(cpqd_print PROPERTIES
    LANGUAGE C
    )
