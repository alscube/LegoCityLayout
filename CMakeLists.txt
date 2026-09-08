cmake_minimum_required(VERSION 3.21)

project(LegoCityLayout VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(LegoCityLayout
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/CityLayout.cpp
    src/CityLayout.h
    src/LayoutElementWidget.cpp
    src/LayoutElementWidget.h
    src/LayoutElementsView.cpp
    src/LayoutElementsView.h
    src/LayoutElementsList.cpp
    src/LayoutElementsList.h
    src/Preferences.cpp
    src/Preferences.h
    resources.qrc
)

target_link_libraries(LegoCityLayout PRIVATE Qt6::Widgets)

set_target_properties(LegoCityLayout PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LegoCityLayout
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
