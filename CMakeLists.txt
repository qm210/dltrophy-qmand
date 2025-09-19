cmake_minimum_required(VERSION 3.31)
project(dltrophy_qmand)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conan_toolchain.cmake)
find_package(nuklear REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

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

target_link_libraries(${PROJECT_NAME}
        nuklear::nuklear
        glfw
        OpenGL::GL
)

if(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread dl m)
endif()

target_link_options(${PROJECT_NAME} PRIVATE
        -static -static-libgcc -static-libstdc++
)
