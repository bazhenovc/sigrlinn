
cmake_minimum_required(VERSION 2.8)
project(sigrlinn)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(D3D11 REQUIRED)

file(GLOB src          sigrlinn/*.cc)
file(GLOB hdr          sigrlinn/*.hh)
file(GLOB src_internal sigrlinn/private/*.cc)
file(GLOB hdr_internal sigrlinn/private/*.hh)

file(GLOB demo_common_src demo/common/*.cc)
file(GLOB demo_common_hdr demo/common/*.hh)

file(GLOB demo_src demo/*.cc)
file(GLOB demo_hdr demo/*.hh)

source_group("source"            FILES ${src})
source_group("include"           FILES ${hdr})
source_group("source\\internal"  FILES ${src_internal})
source_group("include\\internal" FILES ${hdr_internal})
source_group("glew"              FILES sigrlinn/GL/glew.c sigrlinn/GL/glew.h sigrlinn/GL/glxew.h sigrlinn/GL/wglew.h)

source_group("demo\\common\\source"  FILES ${demo_common_src})
source_group("demo\\common\\include" FILES ${demo_common_hdr})

source_group("demo\\source"  FILES ${demo_src})
source_group("demo\\include" FILES ${demo_hdr})

set(SGFX_GLEW_SRC
    sigrlinn/GL/glew.c
    sigrlinn/GL/glew.h
    sigrlinn/GL/glxew.h
    sigrlinn/GL/wglew.h
)

include_directories(${CMAKE_SOURCE_DIR}/sigrlinn)
include_directories(${CMAKE_SOURCE_DIR}/external/glm/)

add_library(SigrlinnD3D11 ${hdr} ${hdr_internal} ${src_internal} sigrlinn/sigrlinn_d3d11.cc)
add_library(SigrlinnGL4 ${hdr} ${hdr_internal} ${src_internal} ${SGFX_GLEW_SRC} sigrlinn/sigrlinn_gl4.cc)

function(AddDemo Name Source)

    add_executable(${Name}D3D11 WIN32 ${Source} ${demo_common_src} ${demo_common_hdr} demo/win32_app.cc)
    target_link_libraries(${Name}D3D11 SigrlinnD3D11)

    #add_executable(${Name}GL4 ${Source} ${demo_common_src} ${demo_common_hdr} demo/win32_app.cc)
    #target_link_libraries(${Name}GL4 SigrlinnGL4)
endfunction()

AddDemo(GrassDemo demo/demo_grass.cc)
AddDemo(CubeDemo demo/demo_cube.cc)
AddDemo(PBR demo/demo_pbr.cc)
AddDemo(Terrain demo/demo_terrain.cc)
