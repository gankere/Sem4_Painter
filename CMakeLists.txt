cmake_minimum_required(VERSION 3.16)
project(PainterQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)
qt_standard_project_setup()

qt_add_executable(PainterQt
    main.cpp
    mainwindow.h mainwindow.cpp
    qtcanvaswidget.h qtcanvaswidget.cpp
    domain.h domain.cpp
    canvasdialog.h canvasdialog.cpp
)

target_link_libraries(PainterQt PRIVATE Qt6::Widgets Qt6::Concurrent)

# АВТОМАТИЧЕСКОЕ КОПИРОВАНИЕ ИКОНОК ПРИ СБОРКЕ
add_custom_command(TARGET PainterQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/icons"
        "$<TARGET_FILE_DIR:PainterQt>/icons"
    COMMENT "Copying icons to build directory..."
)