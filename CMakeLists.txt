cmake_minimum_required(VERSION 3.19)
project(client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Sql Multimedia)

qt_standard_project_setup()

qt_add_executable(client
    main.cpp
    client.h client.cpp
    logindialog.h logindialog.cpp
    registerdialog.h registerdialog.cpp
    mainwindow.h mainwindow.cpp
)

target_link_libraries(client
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Multimedia
)
