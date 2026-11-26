#pragma once

#include "golarion/play/identity_save_data.hpp"

#include <filesystem>

namespace golarion::play::detail
{
    IdentitySaveData readIdentityState(const std::filesystem::path &path);
    void writeIdentityState(const std::filesystem::path &path, const IdentitySaveData &data);
}
