set_property(GLOBAL PROPERTY USE_FOLDERS On)
cmake_minimum_required(VERSION 2.8.3) 

project(uvfconvert)

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
set(TuvokIncludeDir /home/toand/git/Tuvok/build/include)
include_directories(
	${TuvokIncludeDir}
	${TuvokIncludeDir}/Tuvok
	${TuvokIncludeDir}/Tuvok/Basics
	${TuvokIncludeDir}/Tuvok/Basics/3rdParty
	${TuvokIncludeDir}/Tuvok/IO/exception
)
link_directories( 
	/home/toand/git/Tuvok/build/lib
)

include_directories(
	DebugOut
	../
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	-lTuvokExpressions 
	-lTuvok
)


# Main app
SET( srcs 
	DebugOut/HRConsoleOut.cpp
	main.cpp 
	)

# Headers
SET( headers
)

add_executable(uvfconvert ${srcs} ${headers})
target_link_libraries(uvfconvert ${ALL_LIBS} )
