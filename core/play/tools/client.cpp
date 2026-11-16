#include "golarion/play/session.hpp"
#include "console.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <thread>

namespace
{
    using namespace golarion::play;
    namespace fs = std::filesystem;

    std::optional<UserToken> readToken(const fs::path &path)
    {
        if (!fs::exists(path)) return std::nullopt;
        if (!fs::is_regular_file(path) || fs::file_size(path) > 18) throw std::invalid_argument("invalid client token file");
        std::ifstream file(path, std::ios::binary);
        if (!file) throw std::runtime_error("cannot open client token file");
        std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        if (file.bad()) throw std::runtime_error("cannot read client token file");
        if (text.ends_with("\r\n")) text.resize(text.size() - 2);
        else if (text.ends_with('\n')) text.pop_back();
        return UserToken(text);
    }

    class TokenOutput final
    {
    public:
        explicit TokenOutput(fs::path destination) : destination_(std::move(destination))
        {
            if (destination_.filename().empty()) throw std::invalid_argument("token path must name a file");
            for (int attempt = 0; attempt < 32; ++attempt)
            {
                auto candidate = destination_;
                candidate += ".tmp." + UserToken::generate().value();
                if (fs::exists(candidate)) continue;
                file_.open(candidate, std::ios::binary | std::ios::out);
                if (!file_) throw std::runtime_error("cannot create token temporary file");
                temporary_ = std::move(candidate);
                try { fs::permissions(temporary_, fs::perms::owner_read | fs::perms::owner_write); }
                catch (...)
                {
                    file_.close();
                    std::error_code ignored;
                    fs::remove(temporary_, ignored);
                    throw;
                }
                file_.exceptions(std::ios::failbit | std::ios::badbit);
                return;
            }
            throw std::runtime_error("cannot create an unused token temporary file");
        }
        ~TokenOutput()
        {
            file_.exceptions(std::ios::goodbit);
            if (file_.is_open()) file_.close();
            std::error_code ignored;
            if (!temporary_.empty()) fs::remove(temporary_, ignored);
        }
        TokenOutput(const TokenOutput &) = delete;
        TokenOutput &operator=(const TokenOutput &) = delete;

        void commit(const UserToken &token)
        {
            file_ << token.value() << '\n';
            file_.flush();
            file_.close();
            if (fs::exists(destination_)) throw std::runtime_error("client token file appeared during admission");
            fs::rename(temporary_, destination_);
            temporary_.clear();
        }

    private:
        fs::path destination_;
        fs::path temporary_;
        std::ofstream file_;
    };

    std::string_view roleLabel(Role role)
    {
        switch (role)
        {
            case Role::Host: return "host";
            case Role::Master: return "master";
            case Role::Player: return "giocatore";
            case Role::Spectator: return "spettatore";
            case Role::Any: break;
        }
        throw std::invalid_argument("invalid participant role");
    }

    std::string_view rejectionMessage(ProtocolErrorCode code)
    {
        switch (code)
        {
            case ProtocolErrorCode::InvalidSessionCode: return "Codice della sessione non valido.";
            case ProtocolErrorCode::InvalidUserToken: return "Token non riconosciuto per questa sessione.";
            case ProtocolErrorCode::Forbidden: return "Permessi insufficienti.";
            case ProtocolErrorCode::AdmissionFailed: return "L'host non ha potuto completare l'ingresso.";
            default: return "Risposta dell'host non valida o ingresso rifiutato.";
        }
    }
}

int main(int argc, char *argv[])
{
    using namespace golarion::play;
    using namespace std::chrono_literals;
    if (argc != 4 && argc != 5)
    {
        std::cerr << "Uso: golarion_play_client <indirizzo IPv4> <porta> <codice sessione> [file token]\n";
        return 1;
    }
    try
    {
        const std::string ip = argv[1];
        const auto port = tools::parsePort(argv[2], false);
        const SessionCode code(argv[3]);
        const std::filesystem::path tokenPath = argc == 5 ? argv[4] : "client-token.txt";
        const auto saved = readToken(tokenPath);
        std::optional<TokenOutput> output;
        if (!saved) output.emplace(tokenPath);
        auto session = Session::createClient();
        tools::installStopHandlers();
        auto joining = saved ? session.resumeSession(ip, port, code, *saved) : session.joinSession(ip, port, code);
        while (joining.wait_for(50ms) != std::future_status::ready)
        {
            if (tools::stopRequested) { session.disconnectFromHost(); return 0; }
        }
        const auto admitted = joining.get();
        if (output) output->commit(*admitted.userToken);
        std::cout << "Collegato alla sessione. Utente: " << admitted.userId << std::endl;
        std::cout << "Ruolo: " << roleLabel(admitted.role) << std::endl;
        while (!tools::stopRequested && session.getClientState().state == ClientConnectionState::Connected) std::this_thread::sleep_for(100ms);
        session.disconnectFromHost();
        if (!tools::stopRequested)
        {
            std::cerr << "La connessione con l'host è stata chiusa.\n";
            return 1;
        }
        return 0;
    }
    catch (const ProtocolError &error)
    {
        std::cerr << "Ingresso non riuscito: " << rejectionMessage(error.code()) << '\n';
        return 1;
    }
    catch (const std::system_error &error)
    {
        std::cerr << "Impossibile collegarsi o gestire il file del token (errore di sistema " << error.code().value() << ").\n";
        return 1;
    }
    catch (const std::exception &)
    {
        std::cerr << "Impossibile completare l'ingresso: controlla indirizzo, porta, codice di sessione e file del token.\n";
        return 1;
    }
}
