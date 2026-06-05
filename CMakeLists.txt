# cmake_minimum_required(VERSION 3.16)
# project(painter_lab VERSION 0.1.0 LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

# # Поиск Qt6 (обязательно компонент Widgets)
# find_package(Qt6 REQUIRED COMPONENTS Widgets)

# # Включает AUTOMOC, AUTORCC, AUTOUIC и другие настройки Qt
# qt_standard_project_setup()

# # Генерирует compile_commands.json для корректной работы IntelliSense в VS Code
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add_executable(painter_lab 
#     main.cpp
#     domain.cpp domain.h
#     presentation.cpp presentation.h
#     logger.cpp logger.h
#     qtcanvaswidget.cpp qtcanvaswidget.h
#     mainwindow.cpp mainwindow.h
#     # datasource.cpp datasource.h  # Раскомментируй, когда подключишь сохранение
# )

# # Подключаем библиотеки Qt к исполняемому файлу
# # Автоматически добавляет include-пути и линкует необходимые .dll/.so
# target_link_libraries(painter_lab PRIVATE Qt6::Widgets)

cmake_minimum_required(VERSION 3.16)
project(PainterQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включаем автоматическую обработку Qt-файлов
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(PainterQt
    main.cpp
    mainwindow.h mainwindow.cpp
    qtcanvaswidget.h qtcanvaswidget.cpp
    domain.h domain.cpp
)

target_link_libraries(PainterQt PRIVATE Qt6::Widgets)

# === АВТОМАТИЧЕСКОЕ КОПИРОВАНИЕ ИКОНОК ПРИ СБОРКЕ ===
# Эта команда выполняется ПОСЛЕ успешной компиляции.
# Она берет папку icons из исходников и кладет её рядом с .exe файлом.
add_custom_command(TARGET PainterQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/icons"
        "$<TARGET_FILE_DIR:PainterQt>/icons"
    COMMENT "Copying icons to build directory..."
)