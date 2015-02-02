
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

include_directories(${CMAKE_SOURCE_DIR}/sigrlinn)
include_directories(${CMAKE_SOURCE_DIR}/external/glm/)
add_library(Sigrlinn STATIC
	${demo_common_src}
	${demo_common_hdr}
	${src}
	${hdr}
	${src_internal}
	${hdr_internal}

	# GLEW
	sigrlinn/GL/glew.c
	sigrlinn/GL/glew.h
	sigrlinn/GL/glxew.h
	sigrlinn/GL/wglew.h
)

add_executable(GrassDemo WIN32 demo/demo_grass.cc demo/win32_app.cc)
target_link_libraries(GrassDemo Sigrlinn)

add_executable(Cube WIN32 demo/demo_cube.cc demo/win32_app.cc)
target_link_libraries(Cube Sigrlinn)

add_executable(PBR WIN32 demo/demo_pbr.cc demo/win32_app.cc)
target_link_libraries(PBR Sigrlinn)

#add_executable(NBody WIN32 demo/demo_nbody.cc demo/win32_app.cc)
#target_link_libraries(NBody Sigrlinn)
