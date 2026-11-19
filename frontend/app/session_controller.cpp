#include "session_controller.hpp"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QSaveFile>
#include <QUuid>

#include <chrono>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace
{
    using namespace golarion::play;
    constexpr quint16 defaultPort = 42500;
    constexpr qint64 maximumCampaignFileSize = 128 * 1024;

    QJsonObject readCampaignData(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.size() > maximumCampaignFileSize) return {};
        return QJsonDocument::fromJson(file.readAll()).object();
    }

    bool writeCampaignData(const QString &path, const QJsonObject &data)
    {
        const auto bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
        if (bytes.size() > maximumCampaignFileSize) return false;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner))
            return false;
        return file.write(bytes) == bytes.size() && file.commit();
    }

    QString normalizedCode(const SessionCode &code)
    {
        const auto value = code.value();
        return QString::fromLatin1(value.data(), static_cast<qsizetype>(value.size()));
    }

    SessionCode campaignCode(const QJsonObject &data)
    {
        return SessionCode(data.value(QStringLiteral("code")).toString().toStdString());
    }

    SessionCode openCampaignCode(const QDir &directory)
    {
        const auto manifest = directory.filePath(QStringLiteral("campaign.json"));
        auto data = readCampaignData(manifest);
        if (data.contains(QStringLiteral("code"))) return campaignCode(data);

        QLockFile lock(directory.filePath(QStringLiteral("hosting.lock")));
        lock.setStaleLockTime(0);
        if (!lock.tryLock()) throw std::runtime_error("cannot lock the campaign");
        data = readCampaignData(manifest);
        if (data.contains(QStringLiteral("code"))) return campaignCode(data);
        if (data.value(QStringLiteral("name")).toString().trimmed().isEmpty())
            throw std::invalid_argument("invalid campaign manifest");

        const auto code = SessionCode::generate();
        data.insert(QStringLiteral("code"), normalizedCode(code));
        if (!writeCampaignData(manifest, data))
            throw std::runtime_error("cannot persist the campaign code");
        return code;
    }

    QString rejectionMessage(ProtocolErrorCode code)
    {
        switch (code)
        {
            case ProtocolErrorCode::InvalidSessionCode: return SessionController::tr("Il codice della sessione non è valido.");
            case ProtocolErrorCode::InvalidUserToken: return SessionController::tr("L’host non riconosce l’iscrizione a questa campagna.");
            case ProtocolErrorCode::Forbidden: return SessionController::tr("L’host ha negato l’accesso.");
            case ProtocolErrorCode::AdmissionFailed: return SessionController::tr("L’host non ha potuto completare l’ingresso.");
            default: return SessionController::tr("L’host ha rifiutato l’ingresso o inviato una risposta non valida.");
        }
    }

    QString connectionError(const std::system_error &error)
    {
        if (error.code() == std::errc::timed_out)
            return SessionController::tr("L’host non ha risposto in tempo.");
        return SessionController::tr("Impossibile collegarsi all’host. Controlla indirizzo IP e porta.");
    }

    QStringList invitationAddresses(int port)
    {
        QStringList tailscale;
        QStringList other;
        for (const auto &networkInterface : QNetworkInterface::allInterfaces())
        {
            const auto flags = networkInterface.flags();
            if (!flags.testFlag(QNetworkInterface::IsUp) || flags.testFlag(QNetworkInterface::IsLoopBack)) continue;

            const bool isTailscale = networkInterface.humanReadableName().contains(QStringLiteral("tailscale"), Qt::CaseInsensitive)
                || networkInterface.name().contains(QStringLiteral("tailscale"), Qt::CaseInsensitive);
            auto &addresses = isTailscale ? tailscale : other;
            for (const auto &entry : networkInterface.addressEntries())
            {
                const auto ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol && ip.toIPv4Address() != 0 && !ip.isLoopback())
                    addresses.append(QStringLiteral("%1:%2").arg(ip.toString()).arg(port));
            }
        }
        tailscale.sort();
        other.sort();
        tailscale.append(other);
        tailscale.removeDuplicates();
        if (tailscale.isEmpty()) tailscale.append(QStringLiteral("127.0.0.1:%1").arg(port));
        return tailscale;
    }
}

SessionController::SessionController(QString campaignDirectory) : campaignDirectory_(std::move(campaignDirectory))
{
    loadCampaigns();
    pollTimer_.setInterval(50);
    connect(&pollTimer_, &QTimer::timeout, this, &SessionController::pollSession);
}

QVariantList SessionController::campaigns() const
{
    QVariantList result;
    for (const auto &campaign : campaigns_)
    {
        result.append(QVariantMap{
            {QStringLiteral("name"), campaign.name},
            {QStringLiteral("address"), campaign.address},
            {QStringLiteral("code"), campaign.code}
        });
    }
    return result;
}

QStringList SessionController::hostedCampaigns() const
{
    QStringList names;
    for (const auto &campaign : hostedCampaigns_) names.append(campaign.name);
    return names;
}

bool SessionController::busy() const { return pending_.has_value() || hostingReply_.valid(); }
QString SessionController::activeName() const { return activeName_; }
QString SessionController::errorMessage() const { return errorMessage_; }
bool SessionController::hostedCampaignOpen() const { return openHostedCampaign_.has_value(); }
int SessionController::hostingPort() const { return hosting_ ? hosting_->port : 0; }
QString SessionController::inviteCode() const { return openInviteCode_; }
QStringList SessionController::inviteEndpoints() const { return hosting_ ? hosting_->endpoints : QStringList{}; }
void SessionController::clearError() { setErrorMessage({}); }

void SessionController::copyInvitationText(const QString &text)
{
    if ((!openInviteCode_.isEmpty() && text == openInviteCode_) || (hosting_ && hosting_->endpoints.contains(text)))
        QGuiApplication::clipboard()->setText(text);
}

bool SessionController::createHostedCampaign(const QString &name)
{
    if (busy() || !activeName_.isEmpty()) return false;
    clearError();

    const auto title = name.trimmed();
    if (title.isEmpty())
    {
        setErrorMessage(tr("Inserisci il nome della campagna."));
        return false;
    }
    const HostedCampaign campaign{QUuid::createUuid().toString(QUuid::WithoutBraces), title};
    const QDir directory(campaignDirectory_);
    if (campaignDirectory_.isEmpty() || !QDir().mkpath(campaignDirectory_) || !directory.mkdir(campaign.id))
    {
        setErrorMessage(tr("Impossibile creare la cartella della campagna."));
        return false;
    }
    const auto manifest = directory.filePath(campaign.id + QStringLiteral("/campaign.json"));
    bool saved = false;
    try
    {
        saved = writeCampaignData(manifest, {
            {QStringLiteral("name"), campaign.name},
            {QStringLiteral("code"), normalizedCode(SessionCode::generate())}
        });
    }
    catch (const std::exception &) {}
    if (!saved)
    {
        directory.rmdir(campaign.id);
        setErrorMessage(tr("Impossibile salvare la campagna su questo PC."));
        return false;
    }
    hostedCampaigns_.append(campaign);
    emit hostedCampaignsChanged();
    return true;
}

void SessionController::openHostedCampaign(int campaignIndex)
{
    if (busy() || !activeName_.isEmpty() || campaignIndex < 0 || campaignIndex >= hostedCampaigns_.size()) return;
    clearError();
    const auto &campaign = hostedCampaigns_.at(campaignIndex);
    try
    {
        const QDir directory(QDir(campaignDirectory_).filePath(campaign.id));
        openInviteCode_ = normalizedCode(openCampaignCode(directory));
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile caricare o salvare il codice d’invito della campagna."));
        return;
    }
    openHostedCampaign_ = campaignIndex;
    activeName_ = campaign.name;
    emit activeNameChanged();
    emit sessionOpened();
}

void SessionController::closeHostedCampaign()
{
    if (!openHostedCampaign_ || hosting_ || busy()) return;
    openHostedCampaign_.reset();
    openInviteCode_.clear();
    activeName_.clear();
    emit activeNameChanged();
    emit sessionClosed();
}

void SessionController::startHosting(int port)
{
    if (!openHostedCampaign_ || hosting_ || busy()) return;
    clearError();
    if (port < 0 || port > 65535)
    {
        setErrorMessage(tr("Inserisci una porta valida."));
        return;
    }

    try
    {
        hostingReply_ = std::async(std::launch::async, &SessionController::launchHosting,
            campaignDirectory_, hostedCampaigns_.at(*openHostedCampaign_), port);
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile avviare l’host."));
        return;
    }
    pollTimer_.start();
    emit busyChanged();
}

SessionController::Hosting SessionController::launchHosting(const QString &campaignDirectory, const HostedCampaign &campaign, int port)
{
    const QDir directory(QDir(campaignDirectory).filePath(campaign.id));
    Hosting result;
    result.lock = std::make_unique<QLockFile>(directory.filePath(QStringLiteral("hosting.lock")));
    result.lock->setStaleLockTime(0);
    if (!result.lock->tryLock())
    {
        result.error = result.lock->error() == QLockFile::LockFailedError
            ? tr("Questa campagna è già in hosting su questo PC.")
            : tr("Impossibile accedere alla cartella della campagna.");
        return result;
    }

    try
    {
        const auto manifest = directory.filePath(QStringLiteral("campaign.json"));
        const auto code = campaignCode(readCampaignData(manifest));
        result.session = std::make_unique<Session>(campaign.id.toStdString(), code);
        const auto archive = std::filesystem::u8path(directory.filePath(QStringLiteral("identities.json")).toStdString());
        if (std::filesystem::exists(archive)) result.session->loadIdentityState(archive);
        else result.session->saveIdentityState(archive);
        result.session->registerSessionInfo(campaign.name.toStdString());
    }
    catch (const std::exception &)
    {
        result.error = tr("Impossibile preparare il codice o l’archivio delle identità della campagna.");
    }
    if (result.error.isEmpty())
    {
        try { result.port = result.session->startHosting("0.0.0.0", static_cast<std::uint16_t>(port)); }
        catch (const std::exception &)
        {
            result.error = tr("Impossibile avviare l’host su questa porta. Prova a sceglierne un’altra.");
        }
    }
    if (!result.error.isEmpty())
    {
        result.session.reset();
        result.lock.reset();
        return result;
    }
    result.endpoints = invitationAddresses(result.port);
    return result;
}

void SessionController::pollHosting()
{
    using namespace std::chrono_literals;
    if (hostingReply_.wait_for(0ms) != std::future_status::ready) return;
    pollTimer_.stop();
    try
    {
        auto result = hostingReply_.get();
        if (result.error.isEmpty())
        {
            hosting_.emplace(std::move(result));
            emit hostingChanged();
        }
        else
        {
            setErrorMessage(result.error);
        }
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile avviare l’host."));
    }
    emit busyChanged();
}

void SessionController::stopHosting()
{
    if (!hosting_) return;
    clearError();
    try { hosting_->session->stopHosting(); }
    catch (const std::exception &) { setErrorMessage(tr("Si è verificato un errore durante l’arresto dell’host.")); }
    hosting_.reset();
    emit hostingChanged();
    closeHostedCampaign();
}

void SessionController::enter(const QString &address, const QString &codeText, int campaignIndex)
{
    if (busy() || !activeName_.isEmpty()) return;
    clearError();

    const auto parts = address.trimmed().split(u':');
    const auto ip = parts.front().trimmed();
    bool validPort = true;
    const auto port = parts.size() == 2 ? parts.at(1).toUShort(&validPort) : defaultPort;
    if (ip.isEmpty() || parts.size() > 2 || !validPort || port == 0)
    {
        setErrorMessage(tr("Inserisci un indirizzo IPv4 valido. Puoi indicare la porta dopo i due punti."));
        return;
    }

    std::optional<SessionCode> code;
    try { code.emplace(codeText.trimmed().toStdString()); }
    catch (const std::invalid_argument &)
    {
        setErrorMessage(tr("Il codice deve contenere cinque lettere o cifre."));
        return;
    }
    if (campaignIndex < -1 || campaignIndex >= campaigns_.size()) return;

    const auto endpoint = QStringLiteral("%1:%2").arg(ip).arg(port);
    const auto canonicalCode = normalizedCode(*code);
    if (campaignIndex == -1)
    {
        for (int index = 0; index < campaigns_.size(); ++index)
        {
            const auto &campaign = campaigns_.at(index);
            if (campaign.address != endpoint || campaign.code != canonicalCode) continue;
            campaignIndex = index;
            break;
        }
    }

    try
    {
        admissionReply_ = campaignIndex == -1
            ? session_.joinSession(ip.toStdString(), port, *code)
            : session_.resumeSession(ip.toStdString(), port, *code, campaigns_.at(campaignIndex).token);
    }
    catch (const std::invalid_argument &)
    {
        setErrorMessage(tr("Inserisci un indirizzo IPv4 valido."));
        return;
    }
    catch (const std::system_error &error)
    {
        setErrorMessage(connectionError(error));
        return;
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile avviare la connessione."));
        return;
    }
    pending_ = Admission{endpoint, canonicalCode, campaignIndex, std::nullopt};
    pollTimer_.start();
    emit busyChanged();
}

void SessionController::disconnectSession()
{
    if (activeName_.isEmpty() || openHostedCampaign_) return;
    pollTimer_.stop();
    session_.disconnectFromHost();
    activeName_.clear();
    emit activeNameChanged();
    emit sessionClosed();
}

void SessionController::pollSession()
{
    using namespace std::chrono_literals;
    if (hostingReply_.valid())
    {
        pollHosting();
        return;
    }
    if (!pending_)
    {
        if (session_.getClientState().state == ClientConnectionState::Disconnected)
        {
            setErrorMessage(tr("La connessione con l’host è stata interrotta."));
            disconnectSession();
        }
        return;
    }

    if (admissionReply_.valid())
    {
        if (admissionReply_.wait_for(0ms) != std::future_status::ready) return;
        try { pending_->newToken = admissionReply_.get().userToken; }
        catch (const ProtocolError &error) { failAdmission(rejectionMessage(error.code())); return; }
        catch (const std::system_error &error) { failAdmission(connectionError(error)); return; }
        catch (const std::exception &) { failAdmission(tr("Impossibile completare l’ingresso.")); return; }

        try { infoReply_ = session_.sendCommand("session.info", nlohmann::json::object()); }
        catch (const std::exception &) { finishAdmission({}); }
        return;
    }

    if (infoReply_.wait_for(0ms) != std::future_status::ready) return;
    QString name;
    try
    {
        const auto info = infoReply_.get();
        const auto title = info.find("name");
        if (title != info.end() && title->is_string())
            name = QString::fromStdString(title->get_ref<const std::string &>()).trimmed();
    }
    catch (const std::exception &) {} // A host can omit campaign metadata.
    finishAdmission(name);
}

void SessionController::finishAdmission(const QString &name)
{
    const auto &admission = *pending_;
    auto index = admission.campaignIndex;
    if (index == -1)
    {
        if (!admission.newToken)
        {
            failAdmission(tr("L’host non ha restituito la credenziale per la campagna."));
            return;
        }
        index = static_cast<int>(campaigns_.size());
        campaigns_.append(Campaign{QUuid::createUuid().toString(QUuid::WithoutBraces), admission.address,
            admission.code, tr("Sessione %1").arg(admission.address), *admission.newToken});
    }
    auto &campaign = campaigns_[index];
    campaign.address = admission.address;
    campaign.code = admission.code;
    if (!name.isEmpty()) campaign.name = name;
    const bool saved = saveCampaign(campaign);
    const bool connected = session_.getClientState().state == ClientConnectionState::Connected;

    if (!connected)
    {
        emit campaignsChanged();
        failAdmission(tr("La connessione con l’host è stata interrotta."));
        return;
    }
    activeName_ = campaign.name;
    pending_.reset();
    if (!saved) setErrorMessage(tr("Impossibile salvare la campagna su questo PC."));
    emit campaignsChanged();
    emit activeNameChanged();
    emit busyChanged();
    emit sessionOpened();
}

void SessionController::failAdmission(const QString &message)
{
    pollTimer_.stop();
    session_.disconnectFromHost();
    pending_.reset();
    setErrorMessage(message);
    emit busyChanged();
}

void SessionController::setErrorMessage(const QString &message)
{
    if (errorMessage_ == message) return;
    errorMessage_ = message;
    emit errorMessageChanged();
}

void SessionController::loadCampaigns()
{
    if (campaignDirectory_.isEmpty()) return;
    const QDir directory(campaignDirectory_);
    for (const auto &entry : directory.entryInfoList({QStringLiteral("*.json")}, QDir::Files | QDir::NoSymLinks, QDir::Name))
    {
        const auto data = readCampaignData(entry.filePath());
        const auto address = data.value(QStringLiteral("address")).toString();
        const auto name = data.value(QStringLiteral("name")).toString();
        if (address.isEmpty() || name.isEmpty()) continue;
        try
        {
            const SessionCode code(data.value(QStringLiteral("code")).toString().toStdString());
            const UserToken token(data.value(QStringLiteral("token")).toString().toStdString());
            campaigns_.append(Campaign{entry.completeBaseName(), address, normalizedCode(code), name, token});
        }
        catch (const std::invalid_argument &) {} // Keep unreadable entries on disk.
    }

    for (const auto &entry : directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name))
    {
        const auto id = entry.fileName();
        if (QUuid(id).toString(QUuid::WithoutBraces) != id) continue;
        const auto data = readCampaignData(QDir(entry.filePath()).filePath(QStringLiteral("campaign.json")));
        const auto name = data.value(QStringLiteral("name")).toString().trimmed();
        if (!name.isEmpty()) hostedCampaigns_.append(HostedCampaign{id, name});
    }
}

bool SessionController::saveCampaign(const Campaign &campaign) const
{
    if (campaignDirectory_.isEmpty() || !QDir().mkpath(campaignDirectory_)) return false;
    const QJsonObject data{
        {QStringLiteral("address"), campaign.address},
        {QStringLiteral("code"), campaign.code},
        {QStringLiteral("name"), campaign.name},
        {QStringLiteral("token"), QString::fromStdString(campaign.token.value())}
    };
    return writeCampaignData(QDir(campaignDirectory_).filePath(campaign.id + QStringLiteral(".json")), data);
}
