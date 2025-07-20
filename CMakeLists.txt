cmake_minimum_required(VERSION 3.19)
project(server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)  # Включение автоматической генерации MOC

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

qt_standard_project_setup()

qt_add_executable(server
    main.cpp
    myserver.h
    myserver.cpp
)

target_link_libraries(server
    PRIVATE
        Qt6::Core
        Qt6::Network
)
