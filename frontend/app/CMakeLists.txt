set(app_qml_files
    Main.qml
    pages/CampaignsPage.qml
    pages/HostedCampaignPage.qml
    pages/SavedSheetsPage.qml
    components/ParticipantPermissionsDialog.qml
    components/ParticipantsPanel.qml
)
foreach(file IN LISTS app_qml_files)
    get_filename_component(resource_name "${file}" NAME)
    set_source_files_properties("${file}" PROPERTIES QT_RESOURCE_ALIAS "${resource_name}")
endforeach()

set(session_controller_sources
    controllers/session_controller.hpp
    controllers/session_controller.cpp
)
set(sheet_controller_sources
    controllers/sheet_controller.hpp
    controllers/sheet_controller.cpp
    controllers/sheet_spell_controller.cpp
)

qt_add_executable(golarion_helper WIN32 main.cpp)
# A generator expression prevents multi-config generators from appending Debug/Release.
set_target_properties(golarion_helper PROPERTIES RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}>")
qt_add_qml_module(golarion_helper
    URI Golarion.App
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES ${app_qml_files}
    SOURCES ${session_controller_sources} ${sheet_controller_sources}
)
set_source_files_properties(controllers/sheet_controller.cpp controllers/sheet_spell_controller.cpp PROPERTIES COMPILE_DEFINITIONS QT_NO_KEYWORDS)
target_include_directories(golarion_helper PRIVATE controllers)
target_link_libraries(golarion_helper PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Concurrent
    golarion_play golarion_model golarion_sheet_ui golarion_sheet_uiplugin nlohmann_json::nlohmann_json)
if(MSVC)
    target_compile_options(golarion_helper PRIVATE /utf-8)
endif()

include(GNUInstallDirs)
if(NOT WIN32)
    file(RELATIVE_PATH installed_catalogs "${CMAKE_INSTALL_FULL_BINDIR}" "${CMAKE_INSTALL_FULL_DATADIR}/golarion-helper/catalogs")
    target_compile_definitions(golarion_helper PRIVATE GOLARION_INSTALLED_CATALOGS="${installed_catalogs}")
endif()
install(TARGETS golarion_helper RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT Runtime)
if(WIN32)
    if(MSVC)
        # Install the redistributable DLLs beside the app, not a nested CRT installer.
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT Runtime)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_INSTALL_BINDIR}")
        include(InstallRequiredSystemLibraries)
        set(qt_runtime_option NO_COMPILER_RUNTIME)
    endif()
    qt_generate_deploy_qml_app_script(TARGET golarion_helper OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS ${qt_runtime_option})
    install(SCRIPT "${deploy_script}" COMPONENT Runtime)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING
        AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND EXISTS "/etc/arch-release")
    include("${PROJECT_SOURCE_DIR}/cmake/ArchInstaller.cmake")
endif()

if(BUILD_TESTING)
    qt_add_executable(golarion_sheet_controller_test tests/sheet_controller_test.cpp ${sheet_controller_sources})
    target_link_libraries(golarion_sheet_controller_test PRIVATE Qt6::Qml Qt6::Test Qt6::Concurrent golarion_model nlohmann_json::nlohmann_json)
    add_test(NAME golarion_sheet_controller_test COMMAND golarion_sheet_controller_test)
    qt_add_executable(golarion_session_controller_test
        tests/session_controller_test.cpp ${session_controller_sources} ${sheet_controller_sources}
    )
    set(app_qmldir "${QT_QML_OUTPUT_DIRECTORY}/Golarion/App/qmldir")
    set_source_files_properties("${app_qmldir}" PROPERTIES QT_RESOURCE_ALIAS qmldir)
    qt_add_resources(golarion_session_controller_test app_test_qml
        PREFIX "/qt/qml/Golarion/App"
        FILES ${app_qml_files} "${app_qmldir}"
    )
    target_link_libraries(golarion_session_controller_test PRIVATE
        Qt6::Quick Qt6::QuickControls2 Qt6::Test Qt6::Network Qt6::Concurrent
        golarion_play golarion_model golarion_sheet_ui golarion_sheet_uiplugin nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(golarion_session_controller_test PRIVATE /utf-8)
    endif()
    add_test(NAME golarion_session_controller_test COMMAND golarion_session_controller_test)
    set_tests_properties(golarion_session_controller_test PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()
