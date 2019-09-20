set(UGLIFY uglifyjs)

file(GLOB JS_SOURCES "js/flotr2.min.js"
                     "js/globals.js"
                     "js/hci.js"
                     "js/domain.js"
                     "js/endpoint.js"
                     "js/host.js"
                     "js/lag.js"
                     "js/mirror.js"
                     "js/link.js"
                     "js/main.js"
                     "js/network.js"
                     "js/port.js"
                     "js/server.js"
                     "js/routes.js"
                     "js/recovery.js"
                     "js/logger.js"
                     "js/ui.js"
                     "js/info.js"
                     "js/background.js"
                     "js/proxy_settings.js")
file(GLOB HTML_SOURCES "html/*.html" "html/*.css")
set(JS_TARGET arcui.js)

add_custom_command(
    OUTPUT ${JS_TARGET}
    COMMAND ${UGLIFY} ${JS_SOURCES} -o ${JS_TARGET} --beautify
    DEPENDS ${JS_SOURCES}
    COMMENT "Compressing javascript files ..."
)

add_custom_target(assets ALL
    DEPENDS ${JS_TARGET}
    COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/images ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/scripts ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/../pcap ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ln -sf ${HTML_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}
)
