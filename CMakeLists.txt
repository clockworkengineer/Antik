cmake_minimum_required(VERSION 3.10.2)

set(LIBRARY_NAME "antik")
set(LIBRARY_VERSION 0.1.0)

project(${LIBRARY_NAME} VERSION ${LIBRARY_VERSION} DESCRIPTION "Antik C++ Static Library")

# Add Boost, threads, curl, openssl, zlib libraries

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS filesystem program_options) 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(CURL REQUIRED)

find_package(OpenSSL REQUIRED)

find_package(ZLIB REQUIRED)

# Antik sources and includes

file(GLOB ANTIK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/classes/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/utility/*.cpp")
file(GLOB ANTIK_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.hpp")

# Antik library

add_library(${PROJECT_NAME} STATIC ${ANTIK_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation)
target_link_libraries(${PROJECT_NAME} Boost::program_options Boost::filesystem Threads::Threads curl OpenSSL::SSL OpenSSL::Crypto ${ZLIB_LIBRARIES} )

#install(TARGETS ${LIBRARY_NAME} DESTINATION ${LIBRARY_NAME}/lib)
#install(FILES ${ANTIK_INCLUDES} DESTINATION ${LIBRARY_NAME}/include)

