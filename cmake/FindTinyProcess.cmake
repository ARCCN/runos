set(TinyProcess_ROOT "${TinyProcess_ROOT}" CACHE PATH
    "Directory to search for tiny process library")

find_library(TinyProcess_LIBRARY
    NAMES tiny-process-library
    HINTS "${TinyProcess_ROOT}"
    )

find_path(TinyProcess_INCLUDE_DIR
    NAMES process.hpp
    HINTS "${TinyProcess_ROOT}"
    PATH_SUFFIXES include
    )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TinyProcess
    DEFAULT_MSG
    TinyProcess_LIBRARY
    TinyProcess_INCLUDE_DIR
    )

if(TinyProcess_FOUND AND NOT TARGET TinyProcess::TinyProcess)
    add_library(TinyProcess::TinyProcess UNKNOWN IMPORTED)

    set_target_properties(TinyProcess::TinyProcess PROPERTIES
        IMPORTED_LOCATION "${TinyProcess_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${TinyProcess_INCLUDE_DIR}"
    )

    mark_as_advanced(TinyProcess_ROOT)
endif()

mark_as_advanced(
    TinyProcess_INCLUDE_DIR
    TinyProcess_LIBRARY)
