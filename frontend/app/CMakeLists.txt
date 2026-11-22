if(NOT TARGET golarion_play)
    function(add_app_play_backend)
        set(BUILD_TESTING OFF)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../core/play" "${CMAKE_CURRENT_BINARY_DIR}/play-backend" EXCLUDE_FROM_ALL)
    endfunction()
    add_app_play_backend()
endif()

find_package(nlohmann_json 3.12 REQUIRED)
qt_add_executable(golarion_helper WIN32 main.cpp)
qt_add_qml_module(golarion_helper
    URI Golarion.App
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES Main.qml CampaignsPage.qml HostedCampaignPage.qml ParticipantsPanel.qml ParticipantPermissionsDialog.qml SavedSheetsPage.qml
    SOURCES session_controller.hpp session_controller.cpp sheet_controller.hpp sheet_controller.cpp sheet_spell_controller.cpp
)
set_source_files_properties(sheet_controller.cpp sheet_spell_controller.cpp PROPERTIES COMPILE_DEFINITIONS QT_NO_KEYWORDS)
target_link_libraries(golarion_helper PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Concurrent
    golarion_play golarion_model golarion_sheet_ui golarion_sheet_uiplugin nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(golarion_helper PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(golarion_sheet_controller_test tests/sheet_controller_test.cpp sheet_controller.cpp sheet_controller.hpp sheet_spell_controller.cpp)
    target_link_libraries(golarion_sheet_controller_test PRIVATE Qt6::Qml Qt6::Test Qt6::Concurrent golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME golarion_sheet_controller_test COMMAND golarion_sheet_controller_test)
    qt_add_executable(golarion_session_controller_test
        tests/session_controller_test.cpp session_controller.cpp session_controller.hpp
    )
    target_link_libraries(golarion_session_controller_test PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Test Qt6::Network golarion_play)
    if(MSVC)
        target_compile_options(golarion_session_controller_test PRIVATE /utf-8)
    endif()
    add_test(NAME golarion_session_controller_test COMMAND golarion_session_controller_test)
    set_tests_properties(golarion_session_controller_test PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
