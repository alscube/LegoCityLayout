cmake_minimum_required(VERSION 3.21)

project(LegoCityLayout VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

set(LAYOUT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/ProjectData.cpp
    src/ProjectData.h
    src/LoadedProjects.cpp
    src/LoadedProjects.h
    src/ProjectView.cpp
    src/ProjectView.h
    src/OpenOrCreateProjectView.cpp
    src/OpenOrCreateProjectView.h
    src/CityLayoutView.cpp
    src/CityLayoutView.h
    src/elements/CityLayoutElement.cpp
    src/elements/CityLayoutElement.h
    src/elements/CityLayoutElements.cpp
    src/elements/CityLayoutElements.h
    src/TableDefinition.cpp
    src/TableDefinition.h
    src/TableDefinitionEditorView.cpp
    src/TableDefinitionEditorView.h
    src/TableSurface.cpp
    src/TableSurface.h
    src/LegoGrid.h
    src/elements/LayoutElement.cpp
    src/elements/LayoutElement.h
    src/elements/LayoutElementsView.cpp
    src/elements/LayoutElementsView.h


    src/UserSettings.cpp
    src/UserSettings.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/SettingsDialog.ui
    src/elements/StraightTrack.cpp
    src/elements/StraightTrack.h
    src/elements/CurvedTrack.cpp
    src/elements/CurvedTrack.h
    src/elements/LeftSwitchTrack.cpp
    src/elements/LeftSwitchTrack.h
    src/elements/BasePlate16x32Green.cpp
    src/elements/BasePlate16x32Green.h
    src/elements/BasePlate32x32Green.cpp
    src/elements/BasePlate32x32Green.h
    src/elements/BasePlate48x48Gray.cpp
    src/elements/BasePlate48x48Gray.h
    src/elements/TestPlate.cpp
    src/elements/TestPlate.h
    src/elements/RightSwitchTrack.cpp
    src/elements/RightSwitchTrack.h
    src/elements/FlexTrack.cpp
    src/elements/FlexTrack.h
    resources.qrc
)

qt_add_executable(LegoCityLayout ${LAYOUT_SOURCES})

target_include_directories(LegoCityLayout PRIVATE src)

target_link_libraries(LegoCityLayout PRIVATE Qt6::Widgets Qt6::Svg)

set_target_properties(LegoCityLayout PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS LegoCityLayout
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

option(LEGO_BUILD_TESTS "Build track connection integration checks" OFF)
if(LEGO_BUILD_TESTS)
    enable_testing()
    set(TEST_LAYOUT_SOURCES ${LAYOUT_SOURCES})
    list(REMOVE_ITEM TEST_LAYOUT_SOURCES src/main.cpp)
    qt_add_executable(switch_connections_check tests/switch_connections.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(switch_connections_check PRIVATE src)
    target_link_libraries(switch_connections_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME switch_connections COMMAND switch_connections_check -platform offscreen)
endif()
