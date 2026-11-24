set(arch_packaging "${PROJECT_SOURCE_DIR}/packaging/arch")
set(arch_workdir "${PROJECT_BINARY_DIR}/packaging/arch")
configure_file("${arch_packaging}/PKGBUILD.in" "${arch_workdir}/PKGBUILD" @ONLY)

install(FILES "${arch_packaging}/org.golarionhelper.GolarionHelper.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications" COMPONENT Runtime)
set(assets "${PROJECT_SOURCE_DIR}/frontend/sheet/assets")
install(FILES "${assets}/ASSETS.md" "${assets}/condition-icons/CREDITS.txt"
    "${assets}/condition-icons/LICENSE-Apache-2.0.txt" "${assets}/condition-icons/LICENSE-Lucide.txt"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/golarion-helper" COMPONENT Runtime)

add_custom_target(installer
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/packages"
    COMMAND "${CMAKE_COMMAND}" -E env "PKGDEST=${PROJECT_BINARY_DIR}/packages"
        "GOLARION_BUILD_DIR=${PROJECT_BINARY_DIR}" "GOLARION_BUILD_CONFIG=$<CONFIG>"
        "GOLARION_CMAKE=${CMAKE_COMMAND}" makepkg --force --nodeps
    WORKING_DIRECTORY "${arch_workdir}"
    USES_TERMINAL VERBATIM)
add_dependencies(installer golarion_helper)
