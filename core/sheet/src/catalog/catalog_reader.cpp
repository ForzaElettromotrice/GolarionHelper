#include "golarion/catalog/catalog_reader.hpp"

#include <fstream>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace golarion::catalog
{
    namespace
    {
        std::mutex directoryMutex;
        std::filesystem::path catalogDirectory = GOLARION_BUILD_CATALOG_DIRECTORY;

        std::string displayPath(const std::filesystem::path &path)
        {
            const auto utf8 = path.u8string();
            return {utf8.begin(), utf8.end()};
        }
    }

    void setDirectory(std::filesystem::path directory)
    {
        const std::scoped_lock lock(directoryMutex);
        catalogDirectory = std::move(directory);
    }

    nlohmann::json readFile(const std::filesystem::path &relativePath)
    {
        const auto directory = [] {
            const std::scoped_lock lock(directoryMutex);
            return catalogDirectory;
        }();
        const auto path = directory / relativePath;
        std::ifstream file(path, std::ios::binary);
        if (!file) throw CatalogReadError("Cannot open catalog: " + displayPath(path));
        try { return nlohmann::json::parse(file); }
        catch (const nlohmann::json::exception &error)
        {
            throw CatalogReadError("Invalid catalog JSON in " + displayPath(path) + ": " + error.what());
        }
    }

    nlohmann::json readCatalog(std::initializer_list<const char *> files, CatalogEntries entries)
    {
        auto catalog = nlohmann::json::array();
        for (const auto *filename : files)
        {
            auto data = readFile(filename);
            if (entries == CatalogEntries::Objects && data.is_object()) catalog.push_back(std::move(data));
            else if (entries == CatalogEntries::Array && data.is_array())
            {
                for (auto &entry : data) catalog.push_back(std::move(entry));
            }
            else throw CatalogReadError("Unexpected catalog JSON type in " + std::string(filename));
        }
        return catalog;
    }
}
