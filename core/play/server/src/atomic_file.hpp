#pragma once

#include <filesystem>
#include <string>

namespace golarion::play::detail
{
    // On failure the destination has not been replaced.
    void writeAtomicFile(const std::filesystem::path &path, const std::string &document);
}
