#include "golarion/play/network.hpp"
#include "console.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <system_error>

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    if (argc > 4)
    {
        std::cerr << "Uso: golarion_play_host [porta] [indirizzo IPv4] [nome partita]\n";
        return 1;
    }
    try
    {
        const std::uint16_t port = argc > 1 ? tools::parsePort(argv[1], true) : 42500;
        const std::string address = argc > 2 ? argv[2] : "127.0.0.1";
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
