#pragma once

#if defined(_WIN32)
    #if defined(GH_BUILDING_LIBRARY)
        #define GH_API __declspec(dllexport)
    #else
        #define GH_API __declspec(dllimport)
    #endif
#else
    #define GH_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct GhCharacterSheet GhCharacterSheet;

GH_API GhCharacterSheet *gh_character_sheet_create(void);
GH_API void gh_character_sheet_destroy(GhCharacterSheet *sheet);

#ifdef __cplusplus
}
#endif
