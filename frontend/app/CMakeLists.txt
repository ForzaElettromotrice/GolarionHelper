if(NOT TARGET golarion_play)
    function(add_app_play_backend)
        set(BUILD_TESTING OFF)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../core/play" "${CMAKE_CURRENT_BINARY_DIR}/play-backend" EXCLUDE_FROM_ALL)
    endfunction()
    add_app_play_backend()
endif()

qt_add_executable(golarion_helper WIN32 main.cpp)
qt_add_qml_module(golarion_helper
    URI Golarion.App
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES Main.qml CampaignsPage.qml HostedCampaignPage.qml ParticipantsPanel.qml
    SOURCES session_controller.hpp session_controller.cpp
)
target_link_libraries(golarion_helper PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network golarion_play)
if(MSVC)
    target_compile_options(golarion_helper PRIVATE /utf-8)
endif()

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
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
