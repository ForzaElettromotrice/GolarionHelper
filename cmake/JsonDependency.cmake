find_package(nlohmann_json 3.12 CONFIG QUIET)
if(TARGET nlohmann_json::nlohmann_json)
    return()
endif()

include(FetchContent)
if(NOT GOLARION_ALLOW_DOWNLOADS)
    if(NOT EXISTS "${FETCHCONTENT_BASE_DIR}/golarion_json_3_12_0-src/CMakeLists.txt")
        message(FATAL_ERROR "nlohmann_json 3.12+ was not found and downloads are disabled. Install the package or enable GOLARION_ALLOW_DOWNLOADS.")
    endif()
    # Reuse a previously populated source tree without contacting the network.
    set(FETCHCONTENT_SOURCE_DIR_GOLARION_JSON_3_12_0 "${FETCHCONTENT_BASE_DIR}/golarion_json_3_12_0-src")
endif()

FetchContent_Declare(golarion_json_3_12_0
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    TLS_VERIFY ON
)
FetchContent_MakeAvailable(golarion_json_3_12_0)
