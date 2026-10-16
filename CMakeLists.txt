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
    src/elements/ElementCatalog.cpp
    src/elements/ElementCatalog.h
    src/elements/CityLayoutTrack.cpp
    src/elements/CityLayoutTrack.h
    src/elements/TrackRun.cpp
    src/elements/TrackRun.h
    src/CityLayoutElements.cpp
    src/CityLayoutElements.h
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
#    src/elements/StraightTrack.cpp
#    src/elements/StraightTrack.h
#    src/elements/CrosswalkPlate.cpp
#    src/elements/CrosswalkPlate.h
#    src/elements/RoadIntersectionPlate.cpp
#    src/elements/RoadIntersectionPlate.h
#    src/elements/RoadTurnPlate.cpp
#    src/elements/RoadTurnPlate.h
#    src/elements/StraightRoadPlate.cpp
#    src/elements/StraightRoadPlate.h
#    src/elements/CurvedTrack.cpp
#    src/elements/CurvedTrack.h
#    src/elements/LeftSwitchTrack.cpp
#    src/elements/LeftSwitchTrack.h
#    src/elements/RightSwitchTrack.cpp
#    src/elements/RightSwitchTrack.h


    resources.qrc
)


qt_add_executable(LegoCityLayout ${LAYOUT_SOURCES})

target_include_directories(LegoCityLayout PRIVATE src src/elements)
target_compile_definitions(LegoCityLayout PRIVATE LEGO_PLATES_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Elements/Plates")
target_compile_definitions(LegoCityLayout PRIVATE LEGO_BUILDINGS_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Elements/Buildings")

# Keep editable plate definitions next to packaged applications as well.
if(APPLE)
    set(ELEMENTS_RUNTIME_DIRECTORY "$<TARGET_BUNDLE_DIR:LegoCityLayout>/Contents/Resources/Elements")
else()
    set(ELEMENTS_RUNTIME_DIRECTORY "$<TARGET_FILE_DIR:LegoCityLayout>/Elements")
endif()
add_custom_target(copy_elements ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Elements" "${ELEMENTS_RUNTIME_DIRECTORY}"
    DEPENDS LegoCityLayout
    VERBATIM
)

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

if(APPLE)
    install(DIRECTORY Elements DESTINATION "LegoCityLayout.app/Contents/Resources")
else()
    install(DIRECTORY Elements DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

option(LEGO_BUILD_TESTS "Build track connection integration checks" OFF)
if(LEGO_BUILD_TESTS)
    enable_testing()
    set(TEST_LAYOUT_SOURCES ${LAYOUT_SOURCES})
    list(REMOVE_ITEM TEST_LAYOUT_SOURCES src/main.cpp)
    qt_add_executable(switch_connections_check tests/switch_connections.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(switch_connections_check PRIVATE src src/elements)
    target_compile_definitions(switch_connections_check PRIVATE LEGO_PLATES_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Elements/Plates")
    target_link_libraries(switch_connections_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME switch_connections COMMAND switch_connections_check -platform offscreen)

    qt_add_executable(missing_elements_check tests/missing_elements.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(missing_elements_check PRIVATE src src/elements)
    get_target_property(APP_ELEMENT_DEFINITIONS LegoCityLayout COMPILE_DEFINITIONS)
    target_compile_definitions(missing_elements_check PRIVATE ${APP_ELEMENT_DEFINITIONS})
    target_link_libraries(missing_elements_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME missing_elements COMMAND missing_elements_check -platform offscreen)

    qt_add_executable(table_editor_placements_check tests/table_editor_placements.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(table_editor_placements_check PRIVATE src src/elements)
    target_compile_definitions(table_editor_placements_check PRIVATE ${APP_ELEMENT_DEFINITIONS})
    target_link_libraries(table_editor_placements_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME table_editor_placements COMMAND table_editor_placements_check -platform offscreen)
    qt_add_executable(horizontal_plates_check tests/horizontal_plates.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(horizontal_plates_check PRIVATE src src/elements)
    target_compile_definitions(horizontal_plates_check PRIVATE ${APP_ELEMENT_DEFINITIONS})
    target_link_libraries(horizontal_plates_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME horizontal_plates COMMAND horizontal_plates_check -platform offscreen)
    qt_add_executable(deletion_undo_check tests/deletion_undo.cpp ${TEST_LAYOUT_SOURCES})
    target_include_directories(deletion_undo_check PRIVATE src src/elements)
    target_compile_definitions(deletion_undo_check PRIVATE ${APP_ELEMENT_DEFINITIONS})
    target_link_libraries(deletion_undo_check PRIVATE Qt6::Widgets Qt6::Svg)
    add_test(NAME deletion_undo COMMAND deletion_undo_check -platform offscreen)
endif()
