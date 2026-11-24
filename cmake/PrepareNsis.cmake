cmake_minimum_required(VERSION 3.25)

# Also usable by a future packaging script that needs the absolute executable.
function(golarion_find_nsis output)
    find_program(installed_nsis NAMES makensis.exe
        HINTS "[HKEY_LOCAL_MACHINE/SOFTWARE/NSIS;]" "[HKEY_CURRENT_USER/SOFTWARE/NSIS;]"
        PATHS "$ENV{ProgramFiles}/NSIS" "$ENV{ProgramFiles\(x86\)}/NSIS"
        REGISTRY_VIEW BOTH NO_CACHE)
    if(installed_nsis)
        set(${output} "${installed_nsis}" PARENT_SCOPE)
        return()
    endif()

    set(version 3.13)
    set(tool "${NSIS_TOOLS_DIR}/nsis-${version}/makensis.exe")
    if(NOT EXISTS "${tool}")
        if(NOT GOLARION_ALLOW_DOWNLOADS)
            message(FATAL_ERROR "NSIS is missing and downloads are disabled. Install NSIS or enable GOLARION_ALLOW_DOWNLOADS.")
        endif()
        file(MAKE_DIRECTORY "${NSIS_TOOLS_DIR}")
        set(archive "${NSIS_TOOLS_DIR}/nsis-${version}.zip")
        message(STATUS "Preparing build-local NSIS ${version}")
        file(DOWNLOAD "https://downloads.sourceforge.net/project/nsis/NSIS%203/${version}/nsis-${version}.zip"
            "${archive}" EXPECTED_HASH SHA256=ba63dffc4410ee89193e1cb5a41989991bd77c61068da17e3156d136b7b0b3d8
            TLS_VERIFY ON INACTIVITY_TIMEOUT 90)
        file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${NSIS_TOOLS_DIR}/staging")
        file(RENAME "${NSIS_TOOLS_DIR}/staging/nsis-${version}" "${NSIS_TOOLS_DIR}/nsis-${version}")
    endif()
    set(${output} "${tool}" PARENT_SCOPE)
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    golarion_find_nsis(nsis)
    message(STATUS "NSIS compiler: ${nsis}")
endif()
