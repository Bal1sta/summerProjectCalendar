cmake_minimum_required(VERSION 3.19)
project(summerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Multimedia)

qt_standard_project_setup()

qt_add_resources(RESOURCES resources.qrc)

set(SOURCES
    main.cpp
    mainwindow.cpp
    notesdatabase.cpp
    thememanager.cpp
    clienthttp.cpp
    client.cpp
    logindialog.cpp
    registerdialog.cpp
)

set(HEADERS
    mainwindow.h
    notesdatabase.h
    thememanager.h
    clienthttp.h
    client.h
    logindialog.h
    registerdialog.h
)

if(WIN32)
    qt_add_executable(summerProject WIN32
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
elseif(UNIX)
    qt_add_executable(summerProject
        ${SOURCES}
        ${HEADERS}
        ${RESOURCES}
    )
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

target_link_libraries(summerProject
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Multimedia
)
