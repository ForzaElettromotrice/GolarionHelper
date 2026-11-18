#include "local_commands.hpp"

#include <condition_variable>
#include <deque>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <utility>

namespace golarion::play
{
    struct ProtocolManager::LocalCommands::Impl
    {
        struct Request
        {
            std::string name;
            nlohmann::json payload;
            std::promise<nlohmann::json> result;
        };

        ProtocolManager &owner;
        std::mutex mutex;
        std::condition_variable ready;
        std::deque<Request> requests;
        bool stopping = false;
        std::thread worker;

        explicit Impl(ProtocolManager &manager) : owner(manager), worker([this] { run(); }) {}
        ~Impl() { stop(); }

        static std::exception_ptr canceled()
        {
            return std::make_exception_ptr(std::system_error(std::make_error_code(std::errc::operation_canceled), "local command canceled"));
        }

        std::future<nlohmann::json> submit(std::string name, nlohmann::json payload)
        {
            Request request{std::move(name), std::move(payload), {}};
            auto result = request.result.get_future();
            {
                const std::lock_guard lock(mutex);
                if (stopping) throw std::logic_error("local command worker is stopping");
                if (requests.size() >= 64) throw std::length_error("too many queued local commands");
                requests.push_back(std::move(request));
            }
            ready.notify_one();
            return result;
        }

        void stop()
        {
            std::deque<Request> abandoned;
            {
                const std::lock_guard lock(mutex);
                stopping = true;
                abandoned.swap(requests);
            }
            ready.notify_one();
            for (auto &request : abandoned) request.result.set_exception(canceled());
            if (worker.joinable()) worker.join();
        }

        void run() noexcept
        {
            while (true)
            {
                Request request;
                {
                    std::unique_lock lock(mutex);
                    ready.wait(lock, [this] { return stopping || !requests.empty(); });
                    if (stopping) return;
                    request = std::move(requests.front());
                    requests.pop_front();
                }
                try
                {
                    const std::lock_guard executionLock(owner.executionMutex_);
                    {
                        const std::lock_guard lock(mutex);
                        if (stopping) std::rethrow_exception(canceled());
                    }
                    UserId hostId;
                    try { hostId = owner.resources_.invoke<PermissionManager::GetHostIdSignature>("permissions.getHostId"); }
                    catch (...) { throw ProtocolError(ProtocolErrorCode::CommandFailed, "cannot identify the local host"); }
                    request.result.set_value(owner.dispatch(request.name, CommandContext{std::move(hostId)}, request.payload));
                }
                catch (...) { request.result.set_exception(std::current_exception()); }
            }
        }
    };

    ProtocolManager::LocalCommands::LocalCommands(ProtocolManager &owner) : impl_(std::make_unique<Impl>(owner)) {}
    ProtocolManager::LocalCommands::~LocalCommands() = default;
    std::future<nlohmann::json> ProtocolManager::LocalCommands::submit(std::string name, nlohmann::json payload) { return impl_->submit(std::move(name), std::move(payload)); }
    void ProtocolManager::LocalCommands::stop() { impl_->stop(); }
}
