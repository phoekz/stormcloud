cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(stormcloud)

include(FetchContent)
FetchContent_Declare(zstd GIT_REPOSITORY "https://github.com/facebook/zstd.git" GIT_TAG 794ea1b0afca0f020f4e57b6732332231fb23c70 SOURCE_SUBDIR build/cmake) # v1.5.6
set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
FetchContent_MakeAvailable(zstd)

add_executable(stormcloud src/main.c)
target_compile_options(stormcloud PRIVATE /W4 /WX)
target_link_libraries(stormcloud PRIVATE libzstd_static)
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "temp/09KC3164.pak2")
