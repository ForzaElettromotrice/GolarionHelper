qt_add_executable(golarion_sheet_preview main.cpp)
target_compile_definitions(golarion_sheet_preview PRIVATE QT_NO_KEYWORDS)
if(NOT TARGET golarion_model)
    function(add_preview_backend)
        set(BUILD_TESTING OFF)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../core/sheet" "${CMAKE_CURRENT_BINARY_DIR}/sheet-backend" EXCLUDE_FROM_ALL)
    endfunction()
    add_preview_backend()
endif()
qt_add_qml_module(golarion_sheet_preview
    URI Golarion.Preview
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES Main.qml
)
target_link_libraries(golarion_sheet_preview PRIVATE Qt6::Quick Qt6::QuickControls2
    golarion_sheet_ui golarion_sheet_uiplugin golarion_model)
