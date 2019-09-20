# - try to find the google breakpad (custom packaging structure)
#
# Cache Variables: (probably not for direct use in your scripts)
#  Breakpad_INCLUDE_DIR
#  Breakpad_LIBRARY
# Cache Variables to use in your scripts:
#
# Non-cache variables and targets you might use in your CMakeLists.txt:
#  Breakpad_FOUND
#  Breakpad::Breakpad
#
# The following variables can be used to the behavior of the module:
#  Breakpad_ROOT
#   The path to Breakpad installation prefix. If this is specified, this prefix
#   will be searched first while looking for Breakpad

set(Breakpad_ROOT "${Breakpad_ROOT}" CACHE PATH
    "Directory to search for breakpad")

find_library(Breakpad_LIBRARY
    NAMES Breakpad
    HINTS "${Breakpad_ROOT}"
    PATH_SUFFIXES Library/Frameworks
    )

find_path(Breakpad_INCLUDE_DIR
    NAMES breakpad/exception_handler.h
    HINTS "${Breakpad_ROOT}"
    PATH_SUFFIXES include)

find_program(DUMP_SYMS
    NAMES dump_syms
    HINTS "${Breakpad_ROOT}"
    PATH_SUFFIXES bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Breakpad
    DEFAULT_MSG
    Breakpad_LIBRARY
    Breakpad_INCLUDE_DIR
    DUMP_SYMS
    )

if(Breakpad_FOUND AND NOT TARGET Breakpad::Breakpad)
    add_library(Breakpad::Breakpad UNKNOWN IMPORTED)

    set_target_properties(Breakpad::Breakpad PROPERTIES
        FRAMEWORK TRUE
        IMPORTED_LOCATION "${Breakpad_LIBRARY}/Breakpad"
        INTERFACE_INCLUDE_DIRECTORIES "${Breakpad_INCLUDE_DIR}"
    )

    mark_as_advanced(Breakpad_ROOT)
endif()

mark_as_advanced(
    Breakpad_INCLUDE_DIR
    Breakpad_LIBRARY
    Breakpad_EXECUTABLE)
