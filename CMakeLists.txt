cmake_minimum_required(VERSION 3.16)

project(task-manager VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

qt_add_executable(task-manager
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/processeswidget.cpp
    src/processeswidget.h
)

target_link_libraries(task-manager PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
