cmake_minimum_required(VERSION 3.5)

project(www)

file(GLOB SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    index.html
    package.json)
file(GLOB_RECURSE PUBLIC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    public/*)
file(GLOB_RECURSE SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    src/*)

add_custom_command(
    OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/config.js
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/assets/index.css
    ${CMAKE_CURRENT_SOURCE_DIR}/dist/assets/index.js
    DEPENDS ${SOURCES} ${PUBLIC} ${SRC}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND [ ! -d ./node_modules ] && npm install || true
    COMMAND npm run build)

add_custom_target(${PROJECT_NAME} ALL
    SOURCES ${SOURCES} ${PUBLIC} ${SRC}
    DEPENDS
    dist/index.html
    dist/config.js
    dist/assets/index.css
    dist/assets/index.js)

if(SNAPCRAFT_BUILD)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dist/ DESTINATION www)
endif()
