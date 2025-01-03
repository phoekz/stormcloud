cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

project(stormcloud)

#
# Third-party dependencies
#
# Declare.
include(FetchContent)
FetchContent_Declare(zstd GIT_REPOSITORY "https://github.com/facebook/zstd.git" GIT_TAG 794ea1b0afca0f020f4e57b6732332231fb23c70 SOURCE_SUBDIR build/cmake) # v1.5.6
FetchContent_Declare(sdl3 URL "https://github.com/libsdl-org/SDL/releases/download/preview-3.1.6/SDL3-devel-3.1.6-VC.zip") # v3.1.6
FetchContent_Declare(dxc URL "https://globalcdn.nuget.org/packages/microsoft.direct3d.dxc.1.8.2407.12.nupkg") # v1.8.2407.12
FetchContent_Declare(dear_bindings GIT_REPOSITORY "https://github.com/dearimgui/dear_bindings.git" GIT_TAG 139b5b88b49946b817b7f9737d88c5c01b0ce1c8) # Dec 9, 2024
FetchContent_Declare(dear_imgui GIT_REPOSITORY "https://github.com/ocornut/imgui.git" GIT_TAG v1.91.6) # v1.91.6
FetchContent_Declare(stb GIT_REPOSITORY "https://github.com/nothings/stb.git" GIT_TAG 5c205738c191bcb0abc65c4febfa9bd25ff35234) # Nov 9, 2024

# zstd.
set(ZSTD_BUILD_TESTS OFF)
set(ZSTD_BUILD_STATIC ON)
set(ZSTD_BUILD_SHARED OFF)
set(ZSTD_BUILD_PROGRAMS OFF)
set(ZSTD_LEGACY_SUPPORT OFF)
set(ZSTD_MULTITHREAD_SUPPORT OFF)
set(ZSTD_USE_STATIC_RUNTIME ON)
set(ZSTD_BUILD_COMPRESSION OFF)
set(ZSTD_BUILD_DICTBUILDER OFF)
FetchContent_MakeAvailable(zstd)

# sdl3.
FetchContent_MakeAvailable(sdl3)
set(SDL3_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/sdl3-src)

# dxc.
FetchContent_MakeAvailable(dxc)
set(DXC_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/dxc-src/build/native)
set(DXC_BINARY_DIR ${DXC_SOURCE_DIR}/bin/x64)
set(DXC_BINARY ${DXC_BINARY_DIR}/dxc.exe)

if(NOT EXISTS ${DXC_BINARY})
    message(FATAL_ERROR "DXC binary not found at ${DXC_BINARY}")
endif()

# dear_bindings & dear_imgui.
FetchContent_MakeAvailable(dear_bindings)
FetchContent_MakeAvailable(dear_imgui)
set(DEAR_BINDINGS_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/dear_bindings-src)
set(DEAR_BINDINGS_BUILD_DIR ${FETCHCONTENT_BASE_DIR}/dear_bindings-build)
set(DEAR_IMGUI_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/dear_imgui-src)
set(dear_bindings_files
    ${DEAR_BINDINGS_BUILD_DIR}/dcimgui.h
    ${DEAR_BINDINGS_BUILD_DIR}/dcimgui.cpp
    ${DEAR_BINDINGS_BUILD_DIR}/dcimgui_internal.h
    ${DEAR_BINDINGS_BUILD_DIR}/dcimgui_internal.cpp
    ${DEAR_BINDINGS_BUILD_DIR}/imconfig.h
)
set(dear_imgui_files
    ${DEAR_IMGUI_SOURCE_DIR}/imgui.h
    ${DEAR_IMGUI_SOURCE_DIR}/imgui_internal.h
    ${DEAR_IMGUI_SOURCE_DIR}/imgui.cpp
    ${DEAR_IMGUI_SOURCE_DIR}/imgui_demo.cpp
    ${DEAR_IMGUI_SOURCE_DIR}/imgui_draw.cpp
    ${DEAR_IMGUI_SOURCE_DIR}/imgui_tables.cpp
    ${DEAR_IMGUI_SOURCE_DIR}/imgui_widgets.cpp
)
add_custom_command(
    OUTPUT ${dear_bindings_files}
    COMMAND python ${DEAR_BINDINGS_SOURCE_DIR}/dear_bindings.py -o ${DEAR_BINDINGS_BUILD_DIR}/dcimgui ${DEAR_IMGUI_SOURCE_DIR}/imgui.h
    COMMAND python ${DEAR_BINDINGS_SOURCE_DIR}/dear_bindings.py -o ${DEAR_BINDINGS_BUILD_DIR}/dcimgui_internal --include ${DEAR_IMGUI_SOURCE_DIR}/imgui.h ${DEAR_IMGUI_SOURCE_DIR}/imgui_internal.h
    COMMAND ${CMAKE_COMMAND} -E copy ${DEAR_IMGUI_SOURCE_DIR}/imconfig.h ${DEAR_BINDINGS_BUILD_DIR}/imconfig.h
    DEPENDS ${DEAR_IMGUI_SOURCE_DIR}/imgui.h ${DEAR_IMGUI_SOURCE_DIR}/imgui_internal.h
    COMMENT "Copying dear_bindings source files"
)
add_custom_target(dear_bindings DEPENDS ${DEAR_BINDINGS_SOURCE_DIR}/dear_bindings.py)
add_library(dear_imgui STATIC ${dear_bindings_files} ${dear_imgui_files})
add_dependencies(dear_imgui dear_bindings)
target_include_directories(dear_imgui
    PUBLIC ${DEAR_BINDINGS_BUILD_DIR}
    PRIVATE ${DEAR_IMGUI_SOURCE_DIR}
)

# stb.
FetchContent_MakeAvailable(stb)
set(STB_SOURCE_DIR ${FETCHCONTENT_BASE_DIR}/stb-src)
configure_file(cmake/stb.c.in ${STB_SOURCE_DIR}/stb.c COPYONLY)
add_library(stb STATIC ${STB_SOURCE_DIR}/stb.c)
target_include_directories(stb PUBLIC ${STB_SOURCE_DIR})

#
# Shaders
#
set(shader_files
    ${CMAKE_SOURCE_DIR}/src/shaders/hlsl/point.hlsl
    ${CMAKE_SOURCE_DIR}/src/shaders/hlsl/bounds.hlsl
    ${CMAKE_SOURCE_DIR}/src/shaders/hlsl/ddraw.hlsl
    ${CMAKE_SOURCE_DIR}/src/shaders/hlsl/gui.hlsl
)

foreach(shader_file ${shader_files})
    get_filename_component(shader_name ${shader_file} NAME_WE)
    set(vert_file ${CMAKE_SOURCE_DIR}/src/shaders/dxil/${shader_name}.vert)
    set(frag_file ${CMAKE_SOURCE_DIR}/src/shaders/dxil/${shader_name}.frag)
    add_custom_command(OUTPUT ${vert_file} ${frag_file}
        COMMAND ${DXC_BINARY} -T vs_6_0 -E vs_main -Fo ${vert_file} ${shader_file}
        COMMAND ${DXC_BINARY} -T ps_6_0 -E fs_main -Fo ${frag_file} ${shader_file}
        DEPENDS ${shader_file}
        COMMENT "Compiling ${shader_file}"
    )
    list(APPEND vert_files ${vert_file})
    list(APPEND frag_files ${frag_file})
endforeach()

add_custom_target(shaders DEPENDS ${vert_files} ${frag_files})

#
# Stormcloud
#
add_executable(stormcloud
    src/main.c
    src/camera.h
    src/color.h
    src/common.h
    src/ddraw.h
    src/gpu.h
    src/gui.h
    src/math.h
    src/octree.h
)
add_dependencies(stormcloud shaders dear_imgui)
target_compile_options(stormcloud PRIVATE
    /MP
    /W4
    /WX
    /arch:AVX2
    $<$<CONFIG:RELWITHDEBINFO>:/Oi>
    $<$<CONFIG:RELWITHDEBINFO>:/Ot>
    $<$<CONFIG:RELWITHDEBINFO>:/Ob3>
)
target_link_libraries(stormcloud PRIVATE libzstd_static ${SDL3_SOURCE_DIR}/lib/x64/SDL3.lib dear_imgui stb)
target_include_directories(stormcloud PRIVATE ${SDL3_SOURCE_DIR}/include)
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(stormcloud PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "temp/tokyo.oct")
add_custom_command(TARGET stormcloud POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_SOURCE_DIR}/lib/x64/SDL3.dll $<TARGET_FILE_DIR:stormcloud>
    COMMENT "Copying SDL3.dll to $<TARGET_FILE_DIR:stormcloud>"
)
