set(nsis_script "${CMAKE_CURRENT_LIST_DIR}/PrepareNsis.cmake")
set(nsis_tools_dir "${CMAKE_BINARY_DIR}/_deps/nsis")
add_custom_target(prepare_nsis
    COMMAND "${CMAKE_COMMAND}" "-DNSIS_TOOLS_DIR=${nsis_tools_dir}"
        "-DGOLARION_ALLOW_DOWNLOADS=${GOLARION_ALLOW_DOWNLOADS}" -P "${nsis_script}"
    VERBATIM)

set(CPACK_GENERATOR NSIS)
set(CPACK_PACKAGE_FILE_NAME "GolarionHelper-${PROJECT_VERSION}-Setup")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_EXECUTABLES golarion_helper GolarionHelper)
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};Runtime;/")
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/GolarionCPackNSIS.cmake")
configure_file("${CMAKE_CURRENT_LIST_DIR}/CPackNSIS.cmake.in" "${CPACK_PROJECT_CONFIG_FILE}" @ONLY)
include(CPack)

add_custom_target(installer
    COMMAND "${CMAKE_CPACK_COMMAND}" --config "${CPACK_OUTPUT_CONFIG_FILE}" -G NSIS
        "$<$<BOOL:$<CONFIG>>:-C;$<CONFIG>>"
    COMMAND_EXPAND_LISTS VERBATIM)
add_dependencies(installer golarion_helper prepare_nsis)
