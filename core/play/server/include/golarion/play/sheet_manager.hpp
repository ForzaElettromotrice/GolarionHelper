#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <string>

namespace golarion { class CharacterSheet; }

namespace golarion::play
{
    class ServerProtocol;

    class SheetManager final
    {
    public:
        explicit SheetManager(const std::filesystem::path &campaignDirectory);
        ~SheetManager();
        void registerCommands(ServerProtocol &protocol);
        const std::map<std::string, std::string> &namesById() const noexcept { return index_.sheetNames; }

    private:
        enum class Kind { Sheet, Folder };
        struct Index
        {
            std::map<std::string, std::string> sheetNames, folderNames;
            std::map<std::string, std::string> parents; // Root entries have no mapping.
        };
        std::string create(Kind kind, const std::string &name, const std::string &parent);
        void remove(Kind kind, const std::string &id);
        void move(const std::string &id, const std::string &parent);
        void writeIndex(const Index &index) const;

        std::filesystem::path folder_;
        Index index_;
        std::map<std::string, std::unique_ptr<CharacterSheet>> loaded_;
    };
}
