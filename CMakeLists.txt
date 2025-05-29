cmake_minimum_required(VERSION 3.16)

project(pr-btechnique LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти пакеты Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets PrintSupport Charts LinguistTools)

# Автоматическая обработка .ui файлов
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Проверить наличие UI файлов
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/mainwindow.ui")
    set(UI_FILES mainwindow.ui)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/addeditdialog.ui")
    list(APPEND UI_FILES addeditdialog.ui)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/aboutdialog.ui")
    list(APPEND UI_FILES aboutdialog.ui)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/chartdialog.ui")
    list(APPEND UI_FILES chartdialog.ui)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/settings.ui")
    list(APPEND UI_FILES settings.ui)
endif()

# Файлы переводов
set(TS_FILES
    translations/app_ru.ts
    translations/app_de.ts
    translations/app_en.ts
)

# Создание исполняемого файла
add_executable(pr-btechnique
    main.cpp
    equipment.h equipment.cpp
    equipmentmodel.h equipmentmodel.cpp
    mainwindow.h mainwindow.cpp
    addeditdialog.h addeditdialog.cpp
    aboutdialog.h aboutdialog.cpp
    printservice.h printservice.cpp
    settings.h settings.cpp
    chartdialog.h chartdialog.cpp
    ${UI_FILES}
)

# Переводы к исполняемому файлу
qt6_add_translations(pr-btechnique TS_FILES ${TS_FILES})

# Ресурсы после создания переводов
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc")
    qt6_add_resources(pr-btechnique "resources" FILES resources.qrc)
endif()

# Связка с библиотеками Qt
target_link_libraries(pr-btechnique
    Qt6::Core
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Charts
)

# Отладочная информация
message(STATUS "Qt6_FOUND: ${Qt6_FOUND}")
message(STATUS "Qt6Charts_FOUND: ${Qt6Charts_FOUND}")
message(STATUS "Qt6_DIR: ${Qt6_DIR}")
message(STATUS "TS_FILES: ${TS_FILES}")

include(GNUInstallDirs)
install(TARGETS pr-btechnique
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
