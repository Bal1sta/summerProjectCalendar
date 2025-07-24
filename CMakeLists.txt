cmake_minimum_required(VERSION 3.19)
project(summerProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql Multimedia)

qt_standard_project_setup()

qt_add_resources(RESOURCES resources.qrc)

qt_add_executable(summerProject
    WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    notesdatabase.cpp
    notesdatabase.h
    ${RESOURCES}
    thememanager.h thememanager.cpp
    clienthttp.h clienthttp.cpp



)

target_link_libraries(summerProject
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Multimedia
)
