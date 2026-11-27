cmake_minimum_required(VERSION 3.21)
project(PawIDE VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(PAWIDE_BUILD_TESTS "Build unit tests" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Sql Svg Concurrent Network)
if(PAWIDE_BUILD_TESTS)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
endif()

file(GLOB_RECURSE PAWIDE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
list(FILTER PAWIDE_SOURCES EXCLUDE REGEX ".*/src/main\\.cpp$")

add_library(pawide_core STATIC ${PAWIDE_SOURCES} resources/resources.qrc)
target_include_directories(pawide_core PUBLIC src)
target_compile_definitions(pawide_core PUBLIC PAWIDE_VERSION_STR="${PROJECT_VERSION}")
target_link_libraries(pawide_core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Sql Qt6::Svg Qt6::Concurrent Qt6::Network)
if(MSVC)
    target_compile_options(pawide_core PUBLIC /W4 /utf-8 /permissive-)
else()
    target_compile_options(pawide_core PUBLIC -Wall -Wextra)
endif()

add_executable(PawIDE WIN32 src/main.cpp)
target_link_libraries(PawIDE PRIVATE pawide_core)

if(PAWIDE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Developer tool used for documentation screenshots and visual checks.
add_executable(pawide_screenshot tools/screenshot.cpp)
target_link_libraries(pawide_screenshot PRIVATE pawide_core)
