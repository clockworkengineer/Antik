cmake_minimum_required(VERSION 3.10.2)

project("Antik C++ Library" VERSION 0.1.0 DESCRIPTION "Antik C++ Static Library")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

set(ANTIK_LIBRARY_NAME antik)

# Enable unit tests

enable_testing()

# Add Boost, threads, curl, libssh, openssl, zlib libraries

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS filesystem program_options) 

find_package(OpenSSL REQUIRED)

find_package(ZLIB REQUIRED)

find_package(CURL REQUIRED)

find_package(LIBSSH REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Antik sources and includes

file(GLOB ANTIK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/classes/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/utility/*.cpp")
file(GLOB ANTIK_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation/*.hpp")

# Antik library

add_library(${ANTIK_LIBRARY_NAME} STATIC ${ANTIK_SOURCES})
target_include_directories(${ANTIK_LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation)
target_link_libraries(${ANTIK_LIBRARY_NAME} PRIVATE Boost::program_options Boost::filesystem Threads::Threads curl OpenSSL::SSL OpenSSL::Crypto ${ZLIB_LIBRARIES} ssh)

add_subdirectory(tests)

install(TARGETS ${PROJECT_NANTIK_LIBRARY} DESTINATION ${ANTIK_LIBRARY_NAME}/lib)
install(FILES ${ANTIK_INCLUDES} DESTINATION ${ANTIK_LIBRARY_NAME}/include)

