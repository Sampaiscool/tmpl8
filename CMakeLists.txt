cmake_minimum_required(VERSION 3.16)
project(myGame LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(myGame
    game.cpp
    template/opencl.cpp
    template/opengl.cpp
    template/scene.cpp
    template/sprite.cpp
    template/surface.cpp
    template/template.cpp
    template/tmpl8math.cpp
)

set_target_properties(myGame PROPERTIES DEBUG_POSTFIX _debug)

target_include_directories(myGame PRIVATE template . lib/glad lib)

target_precompile_headers(myGame PRIVATE template/precomp.h)

find_package(OpenGL REQUIRED)
find_package(OpenCL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenMP REQUIRED)

target_link_libraries(myGame PRIVATE
    OpenGL::GL
    OpenCL::OpenCL
    glfw
    ZLIB::ZLIB
    OpenMP::OpenMP_CXX
)

if(WIN32)
    set_target_properties(myGame PROPERTIES WIN32_EXECUTABLE ON)
    target_link_libraries(myGame PRIVATE winmm advapi32 user32 gdi32 shell32)
endif()

if(MSVC)
    target_compile_options(myGame PRIVATE /MP /W4 /fp:fast /arch:AVX2)
    target_compile_options(myGame PRIVATE $<$<CONFIG:Release>:/Ox /Ot>)
else()
    target_compile_options(myGame PRIVATE
        -pthread
        -Wall
        -ffast-math
        -fms-extensions
        -Wno-error=format-security
        -Wno-error=sign-compare
        -Wno-error=unused-variable
        -mavx
        -mavx2
    )
endif()
