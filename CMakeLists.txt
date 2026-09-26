cmake_minimum_required(VERSION 3.21)

project(LegoCityLayout VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(LegoCityLayout
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/ProjectView.cpp
    src/ProjectView.h
    src/OpenOrCreateProjectView.cpp
    src/OpenOrCreateProjectView.h
    src/CityLayoutView.cpp
    src/CityLayoutView.h
    src/CityLayoutElement.cpp
    src/CityLayoutElement.h
    src/CityLayoutElements.cpp
    src/CityLayoutElements.h
    src/TableDefinition.cpp
    src/TableDefinition.h
    src/TableDefinitionEditorView.cpp
    src/TableDefinitionEditorView.h
    src/TableSurface.cpp
    src/TableSurface.h
    src/LayoutElement.cpp
    src/LayoutElement.h
    src/LayoutElementsView.cpp
    src/LayoutElementsView.h
    src/LayoutElementsList.cpp
    src/LayoutElementsList.h
    src/UserSettings.cpp
    src/UserSettings.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/SettingsDialog.ui
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
