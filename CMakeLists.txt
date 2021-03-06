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

find_package(Boost COMPONENTS program_options) 

find_package(OpenSSL REQUIRED)

find_package(ZLIB REQUIRED)

find_package(CURL REQUIRED)

find_package(LIBSSH REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Antik sources and includes

set (ANTIK_SOURCES
    ./classes/CApprise.cpp
    ./classes/CCurl.cpp
    ./classes/CFile.cpp
    ./classes/CFTP.cpp
    ./classes/CIMAPBodyStruct.cpp
    ./classes/CIMAP.cpp
    ./classes/CIMAPParse.cpp
    ./classes/CMIME.cpp
    ./classes/CPath.cpp
    ./classes/CRedirect.cpp
    ./classes/CSCP.cpp
    ./classes/CSFTP.cpp
    ./classes/CSMTP.cpp
    ./classes/CSocket.cpp
    ./classes/CSSHChannel.cpp
    ./classes/CSSHSession.cpp
    ./classes/CTask.cpp
    ./classes/CZIP.cpp
    ./classes/CZIPIO.cpp
    ./classes/implementation/CFileEventNotifier.cpp
    ./utility/FTPUtil.cpp
    ./utility/SCPUtil.cpp
    ./utility/SFTPUtil.cpp
    ./utility/SSHChannelUtil.cpp
    ./utility/SSHSessionUtil.cpp
)

set (ANTIK_INCLUDES
    ./include/CApprise.hpp
    ./include/CCurl.hpp
    ./include/CFile.hpp
    ./include/CFTP.hpp
    ./include/CIMAPBodyStruct.hpp
    ./include/CIMAP.hpp
    ./include/CIMAPParse.hpp
    ./include/CMIME.hpp
    ./include/CommonAntik.hpp
    ./include/CommonUtil.hpp
    ./include/CPath.hpp
    ./include/CRedirect.hpp
    ./include/CSCP.hpp
    ./include/CSFTP.hpp
    ./include/CSMTP.hpp
    ./include/CSocket.hpp
    ./include/CSSHChannel.hpp
    ./include/CSSHSession.hpp
    ./include/CTask.hpp
    ./include/CZIP.hpp
    ./include/CZIPIO.hpp
    ./include/FTPUtil.hpp
    ./include/IApprise.hpp
    ./include/SCPUtil.hpp
    ./include/SFTPUtil.hpp
    ./include/SSHChannelUtil.hpp
    ./include/SSHSessionUtil.hpp
)

# Antik library

add_library(${ANTIK_LIBRARY_NAME} STATIC ${ANTIK_SOURCES})
target_include_directories(${ANTIK_LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/classes/implementation)
target_link_libraries(${ANTIK_LIBRARY_NAME} PRIVATE Boost::program_options Threads::Threads curl OpenSSL::SSL OpenSSL::Crypto ${ZLIB_LIBRARIES} ssh)

add_subdirectory(tests)

install(TARGETS ${PROJECT_NANTIK_LIBRARY} DESTINATION ${ANTIK_LIBRARY_NAME}/lib)
install(FILES ${ANTIK_INCLUDES} DESTINATION ${ANTIK_LIBRARY_NAME}/include)

