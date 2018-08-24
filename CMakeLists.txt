cmake_minimum_required(VERSION 3.10.2)

set(LIBRARY_NAME "antik")
set(LIBRARY_VERSION 0.1.0)

project(${LIBRARY_NAME} VERSION ${LIBRARY_VERSION} DESCRIPTION "Antik C++ Static Library")

file(GLOB ANTIK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/classes/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/utility/*.cpp")
file(GLOB ANTIK_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(${PROJECT_NAME} STATIC ${ANTIK_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation)

install(TARGETS ${LIBRARY_NAME} DESTINATION ${LIBRARY_NAME}/lib)
install(FILES ${ANTIK_INCLUDES} DESTINATION ${LIBRARY_NAME}/include)
