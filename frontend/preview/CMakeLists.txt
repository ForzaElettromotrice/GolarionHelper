qt_add_executable(golarion_sheet_preview main.cpp)
target_compile_definitions(golarion_sheet_preview PRIVATE QT_NO_KEYWORDS)
qt_add_qml_module(golarion_sheet_preview
    URI Golarion.Preview
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES Main.qml
)
target_link_libraries(golarion_sheet_preview PRIVATE Qt6::Quick Qt6::QuickControls2
    golarion_sheet_ui golarion_sheet_uiplugin golarion_model)
