cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(stormcloud)

#
# Third-party dependencies
#
include(FetchContent)
FetchContent_Declare(zstd GIT_REPOSITORY "https://github.com/facebook/zstd.git" GIT_TAG 794ea1b0afca0f020f4e57b6732332231fb23c70 SOURCE_SUBDIR build/cmake) # v1.5.6
FetchContent_Declare(sdl3 URL https://github.com/libsdl-org/SDL/releases/download/preview-3.1.6/SDL3-devel-3.1.6-VC.zip) # v3.1.6
FetchContent_Declare(dxcompiler URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.dxc.1.8.2407.12.nupkg") # v1.8.2407.12
set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
FetchContent_MakeAvailable(zstd)
FetchContent_MakeAvailable(sdl3)
set(SDL3_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/sdl3-src)
FetchContent_MakeAvailable(dxcompiler)
set(DXCOMPILER_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/dxcompiler-src/build/native)
set(DXCOMPILER_BINARY_DIR ${DXCOMPILER_SOURCE_DIR}/bin/x64)
set(DXCOMPILER_BINARY ${DXCOMPILER_BINARY_DIR}/dxc.exe)

if(NOT EXISTS ${DXCOMPILER_BINARY})
    message(FATAL_ERROR "DXC binary not found at ${DXCOMPILER_BINARY}")
endif()

#
# Shaders
#
set(shader_files
    ${CMAKE_SOURCE_DIR}/src/shaders/hlsl/basic.hlsl
)

foreach(shader_file ${shader_files})
    get_filename_component(shader_name ${shader_file} NAME_WE)
    set(vert_file ${CMAKE_SOURCE_DIR}/src/shaders/dxil/${shader_name}.vert)
    set(frag_file ${CMAKE_SOURCE_DIR}/src/shaders/dxil/${shader_name}.frag)
    add_custom_command(OUTPUT ${vert_file} ${frag_file}
        COMMAND ${DXCOMPILER_BINARY} -T vs_6_0 -E vs_main -Fo ${vert_file} ${shader_file}
        COMMAND ${DXCOMPILER_BINARY} -T ps_6_0 -E fs_main -Fo ${frag_file} ${shader_file}
        DEPENDS ${shader_file}
        COMMENT "Compiling ${shader_file} to ${vert_file} and ${frag_file}"
    )
    list(APPEND vert_files ${vert_file})
    list(APPEND frag_files ${frag_file})
endforeach()

add_custom_target(shaders DEPENDS ${vert_files} ${frag_files})

#
# Stormcloud
#
add_executable(stormcloud src/main.c)
target_compile_options(stormcloud PRIVATE /W4 /WX)
target_link_libraries(stormcloud PRIVATE libzstd_static ${SDL3_SOURCE_DIR}/lib/x64/SDL3.lib)
target_include_directories(stormcloud PRIVATE ${SDL3_SOURCE_DIR}/include)
add_dependencies(stormcloud shaders)
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "temp/09KC3164.pak2")
add_custom_command(TARGET stormcloud POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_SOURCE_DIR}/lib/x64/SDL3.dll $<TARGET_FILE_DIR:stormcloud>
    COMMENT "Copying SDL3.dll to $<TARGET_FILE_DIR:stormcloud>"
)
