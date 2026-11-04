#include "golarion/play/network.hpp"
#include "console.hpp"

#include <chrono>
#include <iostream>
#include <system_error>

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    if (argc != 4)
    {
        std::cerr << "Uso: golarion_play_client <indirizzo host> <porta> <nome partecipante>\n";
        return 1;
    }
    try
    {
        tools::installStopHandlers();
        GameClient client(argv[1], tools::parsePort(argv[2], false), argv[3]);
        while (!tools::stopRequested)
        {
            const auto snapshot = client.poll(std::chrono::milliseconds(200));
            if (!snapshot) continue;
            std::cout << "Partita: " << snapshot->name << '\n';
            std::cout << "Partecipanti collegati: " << snapshot->connectedParticipants.size() << '\n';
            for (const Participant &participant : snapshot->connectedParticipants)
            {
                std::cout << "  " << participant.id << ": " << participant.name << '\n';
            }
            std::cout << std::flush;
        }
        return 0;
    }
    catch (const std::system_error &error)
    {
        std::cerr << "Impossibile collegarsi all'host (errore di rete " << error.code().value() << ").\n";
        return 1;
    }
    catch (const std::exception &)
    {
        std::cerr << "Collegamento terminato o non disponibile.\n";
        return 1;
    }
}
