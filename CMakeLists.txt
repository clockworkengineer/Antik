cmake_minimum_required(VERSION 2.8)

project(antik)

file(GLOB ANTIK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/classes/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/utility/*.cpp")
file(GLOB ANTIK_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")

add_library(antik STATIC ${ANTIK_SOURCES})

target_include_directories(antik PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation)

install(TARGETS antik DESTINATION lib)
install(FILES ${ANTIK_INCLUDES} DESTINATION include)
