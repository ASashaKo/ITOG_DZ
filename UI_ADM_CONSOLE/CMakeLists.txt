cmake_minimum_required(VERSION 3.12)

project(UI_ADM_CONSOLE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()
if ( MSVC )
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif()

set(CHAT_LIBRARIES)

set(Qt6Modules Core Widgets)

foreach(QtModule IN LISTS Qt6Modules)
 list(APPEND CHAT_LIBRARIES Qt6::${QtModule})
endforeach(QtModule)

find_package(Qt6 COMPONENTS ${Qt6Modules} REQUIRED LinguistTools)

set(PROJECT_SOURCES
    mainwindow.ui
    mainwindow.cpp
    TCPClient.cpp
    User.h
    utils.h
    utils.cpp
    TCPClient.h
    startscreen.ui
    startscreen.h
    startscreen.cpp
    sha1.h
    sha1.cpp
    registrationform.ui
    registrationform.h
    registrationform.cpp
    netcommon.h
    mainwindow.h
    main.cpp
    loginform.ui
    loginform.h
    loginform.cpp
    User.cpp
    Message.h
    Message.cpp
    Logger.h
    Logger.cpp
    clientmainwindow.ui
    clientmainwindow.h
    clientmainwindow.cpp
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt6_wrap_ui(QUI ${UI_FILES})
qt6_add_resources(QRC ${RC_FILES})

add_executable(UI_ADM_CONSOLE ${PROJECT_SOURCES})
target_link_libraries(UI_ADM_CONSOLE PUBLIC ${CHAT_LIBRARIES})

