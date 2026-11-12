#include "golarion/play/session.hpp"
#include "console.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Uso: golarion_play_host <indirizzo IPv4> <porta> [archivio identità]\n";
        return 1;
    }
    try
    {
        const std::string address = argv[1];
        const std::uint16_t port = tools::parsePort(argv[2], true);
        Session session("host");
        const std::filesystem::path archive = argc == 4 ? argv[3] : "session-identities.json";
        if (std::filesystem::exists(archive)) session.loadIdentityState(archive);
        else session.saveIdentityState(archive);
        tools::installStopHandlers();
        const std::uint16_t listeningPort = session.startHosting(address, port);
        std::cout << "In ascolto su " << address << ":" << listeningPort << std::endl;
        std::cout << "Codice sessione: " << session.getSessionCode() << std::endl;
        while (!tools::stopRequested) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        session.stopHosting();
        return 0;
    }
    catch (const std::system_error &error)
    {
        std::cerr << "Impossibile avviare o mantenere l'host (errore di sistema " << error.code().value() << ").\n";
        return 1;
    }
    catch (const std::exception &)
    {
        std::cerr << "Impossibile avviare o mantenere l'host: controlla indirizzo, porta e archivio delle identità.\n";
        return 1;
    }
}
