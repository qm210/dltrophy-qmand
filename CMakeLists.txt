cmake_minimum_required(VERSION 3.31)
project(dltrophy_qmand)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
find_package(nuklear REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(mdns REQUIRED)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/QmandApp.cpp
        src/Config.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
        thirdparty/glad/src/gl.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        thirdparty/
        thirdparty/glad/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        nuklear::nuklear
        glfw
        OpenGL::GL
        mdns::mdns
)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32)
endif()
if (UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl m)
endif()

target_link_options(${PROJECT_NAME} PRIVATE
        -static -static-libgcc -static-libstdc++
)
