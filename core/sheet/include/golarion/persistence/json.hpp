#pragma once

#include "golarion/data/character_sheet_save_data.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace golarion::persistence
{
    std::string toJson(const CharacterSheetSaveData &data);
    CharacterSheetSaveData fromJson(std::string_view json);
    void saveToFile(const CharacterSheetSaveData &data, const std::filesystem::path &path);
    CharacterSheetSaveData loadFromFile(const std::filesystem::path &path);
}
