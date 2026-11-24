#pragma once

#include <filesystem>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace golarion::catalog
{
    enum class CatalogEntries { Array, Objects };
    class CatalogReadError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    // The application sets its installed data directory before starting catalog loading.
    void setDirectory(std::filesystem::path directory);
    nlohmann::json readCatalog(std::initializer_list<const char *> files, CatalogEntries entries);
}
