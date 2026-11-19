#include "golarion/play/session.hpp"
#include "console.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace
{
    std::string sessionNameFromArgument([[maybe_unused]] const char *argument)
    {
#ifdef _WIN32
        int count = 0;
        const std::unique_ptr<wchar_t *, decltype(&LocalFree)> arguments(CommandLineToArgvW(GetCommandLineW(), &count), &LocalFree);
        if (!arguments || count != 5) throw std::runtime_error("cannot read session name argument");
        const auto name = arguments.get()[4];
        const int size = WideCharToMultiByte(CP_UTF8, 0, name, -1, nullptr, 0, nullptr, nullptr);
        if (size == 0) throw std::runtime_error("invalid session name encoding");
        std::string result(size, '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, name, -1, result.data(), size, nullptr, nullptr) == 0)
            throw std::runtime_error("cannot encode session name");
        result.pop_back();
        return result;
#else
        return argument;
#endif
    }
}

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    if (argc < 3 || argc > 5)
    {
        std::cerr << "Uso: golarion_play_host <indirizzo IPv4> <porta> [archivio identità] [nome sessione]\n";
        return 1;
    }
    try
    {
        const std::string address = argv[1];
        const std::uint16_t port = tools::parsePort(argv[2], true);
        auto session = Session::createHost("host");
        const std::filesystem::path archive = argc >= 4 ? argv[3] : "session-identities.json";
        if (argc == 5) session.registerSessionInfo(sessionNameFromArgument(argv[4]));
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
