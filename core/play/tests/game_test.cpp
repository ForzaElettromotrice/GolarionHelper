#include "golarion/play/game.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

namespace
{
    template<typename Function>
    bool rejects(Function function)
    {
        try { function(); }
        catch (const std::invalid_argument &) { return true; }
        return false;
    }
}

int main()
{
    using namespace golarion::play;
    assert(rejects([] { Game game("", "Partita"); }));
    assert(rejects([] { Game game("game", " \t "); }));
    Game game(" game ", " Partita di prova ");
    assert(game.snapshot().id == "game");
    assert(game.snapshot().name == "Partita di prova");
    assert(game.snapshot().connectedParticipants.empty());

    const Participant first = game.connectParticipant(" Alice ");
    const Participant second = game.connectParticipant("Alice");
    assert(first.id != second.id);
    assert(first.name == "Alice");
    assert(game.snapshot().connectedParticipants.size() == 2);
    auto detachedSnapshot = game.snapshot();
    detachedSnapshot.connectedParticipants.clear();
    assert(game.snapshot().connectedParticipants.size() == 2);

    assert(rejects([&game] { game.connectParticipant("\n"); }));
    assert(rejects([&game] { game.connectParticipant("Alice\nBob"); }));
    assert(rejects([&game] { game.connectParticipant(std::string(129, 'a')); }));
    assert(rejects([&game] { game.disconnectParticipant("missing"); }));
    assert(game.snapshot().connectedParticipants.size() == 2);

    game.disconnectParticipant(first.id);
    assert(game.snapshot().connectedParticipants.size() == 1);
    assert(game.snapshot().connectedParticipants[0].id == second.id);
    const Participant third = game.connectParticipant("Bob");
    assert(third.id != first.id);
    game.disconnectParticipant(second.id);
    game.disconnectParticipant(third.id);
    assert(game.snapshot().connectedParticipants.empty());
}
