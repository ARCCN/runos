
# Make final network-setting config file
file(GLOB_RECURSE app_configs
    "*/settings.json"
)
set(default_json "${CMAKE_SOURCE_DIR}/default-settings.json")
set(out_json "${CMAKE_SOURCE_DIR}/runos-settings.json")
set(tmp_json "${CMAKE_SOURCE_DIR}/runos-settings.tmp.json")
execute_process( COMMAND bash -c "cp ${default_json} ${out_json}" )
foreach( _config ${app_configs} )
    execute_process(
        COMMAND bash -c "jq -s '.[0].services = .[0].services + [.[1].name] | \
        if .[1].rest then .[0].services = .[0].services + [.[1].name + \"-rest\"] else . end | \
        if .[1].cli then .[0].services = .[0].services + [.[1].name + \"-cli\"] else . end | \
        .[0] * {(.[1].name): .[1]}' ${out_json} ${_config} > ${tmp_json}"
        OUTPUT_VARIABLE app_name
    )
    file( RENAME ${tmp_json} ${out_json} )
endforeach( _config )

#Macro to get the subdirectory list
macro(SUBDIRLIST result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

#Get all subdirectories (applications)
SUBDIRLIST(apps_dir_list ${CMAKE_CURRENT_SOURCE_DIR})
#list(FILTER apps_dir_list EXCLUDE REGEX "include")

#process all applications: if there is CMakeLists.txt then include it, or add files to current build
foreach(app_dir ${apps_dir_list})
    add_subdirectory(${app_dir})
    target_include_directories(runos SYSTEM
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/${app_dir}/include
    )
    target_include_directories(runos_cli SYSTEM
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/${app_dir}/include
    )
    target_include_directories(runos_rest SYSTEM
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/${app_dir}/include
    )
endforeach()

target_include_directories(runos SYSTEM
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
)

target_include_directories(runos_cli SYSTEM
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
)

target_include_directories(runos_rest SYSTEM
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/core
)
