cmake_minimum_required(VERSION 3.5)

project(Server VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    main.cpp
    ../utils.h
    ../utils.cpp
    ClientConnection.cpp
    ClientConnection.h
    DBAccess.cpp
    DBAccess.h
    Logger.cpp
    Logger.h
    Message.cpp
    Message.h
    TCPServer.cpp
    TCPServer.h
    User.cpp
    User.h
)

include_directories(include
    -I$(/usr/include/mysql/)
    -L$(/usr/lib/x86_64-linux-gnu)
)

add_executable(Server
            ${PROJECT_SOURCES}
        )
        
target_link_libraries(Server PRIVATE
    -lmysqlclient
)

install(TARGETS Server
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

