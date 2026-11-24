cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Configure the desktop application from the repository root")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/QtDependency.cmake")
qt_standard_project_setup(REQUIRES ${golarion_qt_minimum})
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml")

add_subdirectory(sheet)
if(GOLARION_TARGET STREQUAL "golarion_helper")
    add_subdirectory(app)
endif()

if(BUILD_TESTING)
    qt_add_executable(sheet_panels_test tests/main.cpp)
    target_link_libraries(sheet_panels_test PRIVATE Qt6::QuickTest golarion_sheet_ui golarion_sheet_uiplugin)
    add_test(NAME sheet_panels_test COMMAND sheet_panels_test
        -input "${CMAKE_CURRENT_SOURCE_DIR}/tests" -import "${QT_QML_OUTPUT_DIRECTORY}")
    set_tests_properties(sheet_panels_test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
