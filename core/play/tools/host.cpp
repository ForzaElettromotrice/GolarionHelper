#include "golarion/play/network.hpp"
#include "console.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <system_error>

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Uso: golarion_play_host <indirizzo IPv4> <porta> [nome partita]\n";
        return 1;
    }
    try
    {
        const std::string address = argv[1];
        const std::uint16_t port = tools::parsePort(argv[2], true);
        const std::string name = argc > 3 ? argv[3] : "Partita di prova";
        GameHost host(Game("game", name), address, port);
        tools::installStopHandlers();
        std::cout << "In ascolto su " << address << ":" << host.port() << std::endl;
        while (!tools::stopRequested) host.poll(std::chrono::milliseconds(200));
        return 0;
    }
    catch (const std::system_error &error)
    {
        std::cerr << "Impossibile avviare o mantenere l'host (errore di rete " << error.code().value() << ").\n";
        return 1;
    }
    catch (const std::exception &)
    {
        std::cerr << "Impossibile avviare o mantenere l'host: controlla indirizzo e porta.\n";
        return 1;
    }
}
