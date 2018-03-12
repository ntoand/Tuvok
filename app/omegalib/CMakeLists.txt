cmake_minimum_required(VERSION 2.8.1) 
find_package(Omegalib)

# Set the module name here
SET(MODULE_NAME tuvokview)

include_directories(${OMEGA_INCLUDE_DIRS})

include_directories(
	/gpfs/M2Home/toand/Monash063/toand/git/Tuvok/install/include
)
link_directories( 
	/gpfs/M2Home/toand/Monash063/toand/git/Tuvok/install/lib
)

# Set module name and source files here
add_library(${MODULE_NAME} MODULE 
	../Tuvok.h
	../Tuvok.cpp
	tuvokview.cpp
    	)

# Set the module library dependencies here
target_link_libraries(${MODULE_NAME}
        stdc++
        ${OMEGA_LIB}
        python2.7
        -lTuvokExpressions 
		-lTuvok
        )

#------------------------------------------------------------------------------
# DO NOT MODIFY ANYTHING BELOW AFTER THIS LINE
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")
if(WIN32)
        set_target_properties(${MODULE_NAME} PROPERTIES FOLDER modules SUFFIX ".pyd")
endif()

