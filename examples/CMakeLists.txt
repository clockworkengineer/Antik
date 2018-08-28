cmake_minimum_required(VERSION 3.10.2)

set(PROGRAM_NAME "Examples")
set(PROGRAM_VERSION 0.1.0)

project(${PROGRAM_NAME} VERSION ${PROGRAM_VERSION} DESCRIPTION "Antik C++ Library Example Programs")

# Build Antik library

add_subdirectory(../ antik)

# Get example program list

file( GLOB EXAMPLE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" )

# Build each program

foreach( exampleprogram ${EXAMPLE_SOURCES} )
    # I used a simple string replace, to cut off .cpp.
    string( REPLACE ".cpp" "" exampletarget ${exampleprogram} )
    add_executable( ${exampletarget} ${exampleprogram} )
    # Make sure antik is linked to each program
    target_link_libraries( ${exampletarget} antik )
    install(TARGETS ${exampletarget} DESTINATION bin)
endforeach( exampleprogram ${EXAMPLE_SOURCES} )



