cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(GolarionFrontend LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    include(CTest)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.5)
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml")

add_subdirectory(app)
add_subdirectory(sheet)
add_subdirectory(preview)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS QuickTest)
    qt_add_executable(sheet_panels_test tests/main.cpp)
    target_link_libraries(sheet_panels_test PRIVATE Qt6::QuickTest golarion_sheet_ui golarion_sheet_uiplugin)
    add_test(NAME sheet_panels_test COMMAND sheet_panels_test
        -input "${CMAKE_CURRENT_SOURCE_DIR}/tests" -import "${QT_QML_OUTPUT_DIRECTORY}")
    set_tests_properties(sheet_panels_test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
