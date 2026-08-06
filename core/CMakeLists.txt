add_library(golarion_model STATIC
    src/character_sheet.cpp
)

set_target_properties(golarion_model PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(golarion_model
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(golarion SHARED
    src/golarion.cpp
)

target_compile_definitions(golarion PRIVATE GH_BUILDING_LIBRARY)
target_link_libraries(golarion PRIVATE golarion_model)
target_include_directories(golarion
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(golarion_model PRIVATE /W4 /permissive-)
    target_compile_options(golarion PRIVATE /W4 /permissive-)
else()
    target_compile_options(golarion_model PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(golarion PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(golarion_smoke_test tests/golarion_smoke_test.cpp)
    target_link_libraries(golarion_smoke_test PRIVATE golarion)
    add_test(NAME golarion_smoke_test COMMAND golarion_smoke_test)
endif()

