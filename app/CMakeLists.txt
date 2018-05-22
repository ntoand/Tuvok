set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8.3) 

project(volumeview)

message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()


# FIND OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

#find_package(Qt5OpenGL REQUIRED)

# FIND TUVOK
#find_package(Tuvok REQUIRED)
#include_directories(${Tuvok_INCLUDE_DIRS})
#message(${Tuvok_INCLUDE_DIRS})
#message(${Tuvok_LIBRARY_DIRS})
#include_directories(
#	/home/toand/git/Tuvok
#)
link_directories( 
	/home/toand/git/Tuvok/build
)

include_directories(
	external/glfw-3.0.3/include/GLFW/
	external/glm-0.9.4.0/
	external/glew-1.9.0/include/
	../
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	GLFW_303
	GLEW_190
	-lTuvokExpressions 
	-lTuvok
)

add_definitions(
	-DTW_STATIC
	-DTW_NO_LIB_PRAGMA
	-DTW_NO_DIRECT3D
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
)

# Compile external dependencies 
add_subdirectory (external)


# Main app
SET( srcs 
	cJSON.cpp
	GLUtils.cpp
	Camera.cpp
	Tuvok.cpp
	main.cpp 
	)

# Headers
SET( headers
	cJSON.h
	GLInclude.h
	GLUtils.h
	Camera.h
	Tuvok.h
)

add_executable(volumeview ${srcs} ${headers})
target_link_libraries(volumeview ${ALL_LIBS} )
