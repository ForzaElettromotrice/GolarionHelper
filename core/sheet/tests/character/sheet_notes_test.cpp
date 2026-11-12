#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>

int main()
{
    using namespace golarion;
    CharacterSheet sheet;
    assert(sheet.toView().notes.empty());
    assert(sheet.toSaveData().notes.empty());
    const std::string notes = "  Appunti personali\nÈ una promessa.\n\t\"Citazione\" e \\ dettagli\n\n";
    sheet.setNotes(notes);
    const auto snapshot = sheet.toView();
    assert(snapshot.notes == notes);
    assert(sheet.toSaveData().notes == notes);
    assert(persistence::fromJson(persistence::toJson(sheet.toSaveData())).notes == notes);
    const auto path = std::filesystem::temp_directory_path() / "golarion-sheet-notes-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path);
    assert(restored.toView().notes == notes);
    assert(restored.toSaveData().formatVersion == 41);
    restored.setNotes("");
    restored.save(path);
    assert(CharacterSheet::load(path).toView().notes.empty());
    assert(snapshot.notes == notes); // Views are detached snapshots.

    auto legacy = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    legacy["formatVersion"] = 40;
    legacy.erase("notes");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    assert(CharacterSheet::load(path).toSaveData().notes.empty());
    for (int error = 0; error < 3; ++error)
    {
        auto malformed = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
        if (error == 0) malformed["notes"] = nullptr;
        if (error == 1) malformed["notes"] = 42;
        if (error == 2) malformed.erase("notes");
        bool rejected = false;
        try { persistence::fromJson(malformed.dump()); }
        catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
    std::filesystem::remove(path);
}
