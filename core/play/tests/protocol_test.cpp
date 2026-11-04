#include "protocol.hpp"

#include <cassert>
#include <stdexcept>

namespace
{
    bool rejects(std::vector<std::uint8_t> bytes)
    {
        try { static_cast<void>(golarion::play::detail::extractMessage(bytes)); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }
}

int main()
{
    using namespace golarion::play;
    using namespace golarion::play::detail;
    const auto join = encodeJoin("Alice");
    std::vector<std::uint8_t> buffer;
    for (std::size_t index = 0; index < join.size(); ++index)
    {
        buffer.push_back(join[index]);
        const auto message = extractMessage(buffer);
        assert(message.has_value() == (index + 1 == join.size()));
        if (message) assert(std::get<JoinRequest>(*message).name == "Alice");
    }
    assert(buffer.empty());

    const GameSnapshot snapshot{"game", "Partita", {{"first", "Alice"}, {"second", "Bob"}}};
    buffer = encodeSnapshot(snapshot);
    const auto rejected = encodeRejection("Nome non valido.");
    buffer.insert(buffer.end(), rejected.begin(), rejected.end());
    const auto restored = std::get<GameSnapshot>(*extractMessage(buffer));
    assert(restored.id == "game");
    assert(restored.name == "Partita");
    assert(restored.connectedParticipants.size() == 2);
    assert(restored.connectedParticipants[1].name == "Bob");
    assert(std::get<ConnectionRejected>(*extractMessage(buffer)).reason == "Nome non valido.");
    assert(buffer.empty());

    auto invalid = join;
    invalid[4] = 99;
    assert(rejects(invalid));
    invalid = join;
    invalid[5] = 99;
    assert(rejects(invalid));
    invalid = join;
    invalid.push_back(0);
    ++invalid[3];
    assert(rejects(invalid));
    assert(rejects({0, 1, 0, 1}));
    assert(rejects({0, 0, 0, 1}));
    assert(rejects({0, 0, 0, 6, 1, 1, 0, 0, 0, 129}));
}
