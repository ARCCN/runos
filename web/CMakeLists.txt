set(UGLIFY uglifyjs)

file(GLOB JS_SOURCES "js/*")
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
    COMMAND ln -sf ${HTML_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}
)
