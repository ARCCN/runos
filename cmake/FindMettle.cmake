# - try to find the mettle testing framework
#
# Cache Variables: (probably not for direct use in your scripts)
#  Mettle_INCLUDE_DIR
#  Mettle_LIBRARY
# Cache Variables to use in your scripts:
#  Mettle_EXECUTABLE
#
# Non-cache variables and targets you might use in your CMakeLists.txt:
#  Mettle_FOUND
#  Mettle::Mettle
#
# The following variables can be used to the behavior of the module:
#  Mettle_ROOT
#   The path to Mettle installation prefix. If this is specified, this prefix
#   will be searched first while looking for Mettle.ETTLE

set(Mettle_ROOT "${Mettle_ROOT}" CACHE PATH
    "Directory to search for mettle")

find_library(Mettle_LIBRARY
    NAMES mettle
    HINTS "${Mettle_ROOT}"
    PATH_SUFFIXES lib)

find_path(Mettle_INCLUDE_DIR
    NAMES mettle.hpp mettle/header_only.hpp
    HINTS "${Mettle_ROOT}"
    PATH_SUFFIXES include)

find_program(Mettle_EXECUTABLE
    NAMES mettle
    HINTS "${Mettle_ROOT}"
    PATH_SUFFIXES bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mettle
    DEFAULT_MSG
    Mettle_LIBRARY
    Mettle_INCLUDE_DIR
    Mettle_EXECUTABLE
    )

if(Mettle_FOUND AND NOT TARGET Mettle::Mettle)
    find_package(Boost 1.55 REQUIRED COMPONENTS program_options)

    add_library(Mettle::Mettle UNKNOWN IMPORTED)

    set_target_properties(Mettle::Mettle PROPERTIES
        IMPORTED_LOCATION "${Mettle_LIBRARY}"
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON
        INTERFACE_INCLUDE_DIRECTORIES "${Mettle_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES Boost::program_options
    )

    mark_as_advanced(Mettle_ROOT)
endif()

mark_as_advanced(
    Mettle_INCLUDE_DIR
    Mettle_LIBRARY
    Mettle_EXECUTABLE)
