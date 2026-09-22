pkg_check_modules(FLUID_MSG REQUIRED libfluid_msg)

add_library(oxm STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ofdpa/ofdpa20match.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ofdpa/ofpda20match.hh
)

