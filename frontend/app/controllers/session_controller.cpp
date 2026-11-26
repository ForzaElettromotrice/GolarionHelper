#include "session_controller.hpp"
#include "golarion/play/display_name.hpp"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QSaveFile>
#include <QUuid>

#include <chrono>
#include <algorithm>
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

    QString hostedLockPath(const QDir &directory)
    {
        // Keep the lock beside the campaign so the whole directory can be moved on Windows.
        return directory.absolutePath() + QStringLiteral(".hosting.lock");
    }

    SessionCode openCampaignCode(const QDir &directory)
    {
        const auto manifest = directory.filePath(QStringLiteral("campaign.json"));
        auto data = readCampaignData(manifest);
        if (data.contains(QStringLiteral("code"))) return campaignCode(data);

        QLockFile lock(hostedLockPath(directory));
        lock.setStaleLockTime(0);
        if (!lock.tryLock()) throw std::runtime_error("cannot lock the campaign");
        QLockFile legacyLock(directory.filePath(QStringLiteral("hosting.lock")));
        legacyLock.setStaleLockTime(0);
        if (!legacyLock.tryLock()) throw std::runtime_error("cannot lock the campaign archive");
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

SessionController::SessionController(QString campaignDirectory, std::function<bool(const QString &)> moveToTrash)
    : campaignDirectory_(std::move(campaignDirectory)),
      moveToTrash_(moveToTrash ? std::move(moveToTrash) : [](const QString &path) { return QFile::moveToTrash(path); })
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
        const auto view = campaign.toView();
        result.append(QVariantMap{
            {QStringLiteral("id"), QString::fromStdString(campaign.toData().id)},
            {QStringLiteral("name"), QString::fromStdString(view.name)},
            {QStringLiteral("address"), QString::fromStdString(view.address)},
            {QStringLiteral("code"), normalizedCode(view.code)}
        });
    }
    return result;
}

QVariantList SessionController::hostedCampaigns() const
{
    QVariantList result;
    for (const auto &campaign : hostedCampaigns_)
    {
        const auto data = campaign.toData();
        result.append(QVariantMap{
            {QStringLiteral("id"), QString::fromStdString(data.id)},
            {QStringLiteral("name"), QString::fromStdString(data.name)},
            {QStringLiteral("code"), data.code ? normalizedCode(*data.code) : QString{}}
        });
    }
    return result;
}

bool SessionController::busy() const { return pending_.has_value() || hostingReply_.valid(); }
QString SessionController::activeName() const { return activeName_; }
QString SessionController::errorMessage() const { return errorMessage_; }
bool SessionController::hostedCampaignOpen() const { return openHostedCampaign_.has_value(); }
int SessionController::hostingPort() const { return hosting_ ? hosting_->port : 0; }
QString SessionController::inviteCode() const { return openInviteCode_; }
QStringList SessionController::inviteEndpoints() const { return hosting_ ? hosting_->endpoints : QStringList{}; }
QVariantList SessionController::participants() const { return participants_; }
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
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QDir directory(campaignDirectory_);
    if (campaignDirectory_.isEmpty() || !QDir().mkpath(campaignDirectory_) || !directory.mkdir(id))
    {
        setErrorMessage(tr("Impossibile creare la cartella della campagna."));
        return false;
    }
    const auto manifest = directory.filePath(id + QStringLiteral("/campaign.json"));
    std::optional<HostedCampaign> campaign;
    bool saved = false;
    try
    {
        campaign.emplace(HostedCampaignData{id.toStdString(), title.toStdString(), SessionCode::generate()});
        const auto data = campaign->toData();
        saved = writeCampaignData(manifest, {
            {QStringLiteral("name"), QString::fromStdString(data.name)},
            {QStringLiteral("code"), normalizedCode(*data.code)}
        });
    }
    catch (const std::exception &) {}
    if (!saved)
    {
        directory.rmdir(id);
        setErrorMessage(tr("Impossibile salvare la campagna su questo PC."));
        return false;
    }
    hostedCampaigns_.append(std::move(*campaign));
    emit hostedCampaignsChanged();
    return true;
}

bool SessionController::copyHostedCampaignName(const QString &id)
{
    const auto selectedId = id.toStdString();
    const auto campaign = std::find_if(hostedCampaigns_.cbegin(), hostedCampaigns_.cend(), [&](const HostedCampaign &entry)
    {
        return entry.toData().id == selectedId;
    });
    if (campaign == hostedCampaigns_.cend()) return false;

    QGuiApplication::clipboard()->setText(QString::fromStdString(campaign->toView().name));
    return true;
}

bool SessionController::trashHostedCampaign(const QString &id, const QString &confirmationName, const QString &code)
{
    clearError();
    if (busy() || !activeName_.isEmpty() || hosting_)
    {
        setErrorMessage(tr("Chiudi la campagna e ferma l’hosting prima di spostarla nel Cestino."));
        return false;
    }

    const auto selectedId = id.toStdString();
    const auto campaign = std::find_if(hostedCampaigns_.cbegin(), hostedCampaigns_.cend(), [&](const HostedCampaign &entry)
    {
        return entry.toData().id == selectedId;
    });
    if (campaign == hostedCampaigns_.cend() || id.isEmpty() || QUuid(id).toString(QUuid::WithoutBraces) != id)
    {
        setErrorMessage(tr("La campagna selezionata non è più disponibile."));
        return false;
    }
    const auto selected = campaign->toData();
    if (QString::fromStdString(selected.name) != confirmationName || (selected.code ? normalizedCode(*selected.code) : QString{}) != code)
    {
        setErrorMessage(tr("La conferma non corrisponde alla campagna selezionata."));
        return false;
    }

    const auto root = QDir(campaignDirectory_).canonicalPath();
    const auto path = QDir(root).absoluteFilePath(id);
    const QFileInfo folder(path);
    if (campaignDirectory_.isEmpty() || root.isEmpty() || !folder.isDir() || folder.isSymLink() || folder.canonicalFilePath() != path)
    {
        setErrorMessage(tr("Impossibile accedere alla cartella della campagna."));
        return false;
    }

    const QDir directory(path);
    QLockFile lock(hostedLockPath(directory));
    QLockFile legacyLock(directory.filePath(QStringLiteral("hosting.lock")));
    lock.setStaleLockTime(0);
    legacyLock.setStaleLockTime(0);
    if (!lock.tryLock() || !legacyLock.tryLock())
    {
        setErrorMessage(tr("La campagna è in uso o non è accessibile. Chiudila nelle altre finestre e riprova."));
        return false;
    }
    const auto manifest = readCampaignData(directory.filePath(QStringLiteral("campaign.json")));
    if (manifest.value(QStringLiteral("name")).toString().trimmed() != confirmationName
        || manifest.value(QStringLiteral("code")).toString().toUpper() != code)
    {
        setErrorMessage(tr("La campagna salvata è cambiata. Riapri l’app prima di riprovare."));
        return false;
    }
    // The sibling lock still excludes hosting when the old in-directory lock is released.
    legacyLock.unlock();
    if (!moveToTrash_(path))
    {
        setErrorMessage(tr("Impossibile spostare la campagna nel Cestino."));
        return false;
    }
    hostedCampaigns_.removeAt(campaign - hostedCampaigns_.cbegin());
    emit hostedCampaignsChanged();
    return true;
}

bool SessionController::forgetJoinedCampaign(const QString &id)
{
    clearError();
    if (busy() || !activeName_.isEmpty())
    {
        setErrorMessage(tr("Disconnettiti e chiudi la campagna prima di rimuoverla dalla lista."));
        return false;
    }

    const auto selectedId = id.toStdString();
    const auto campaign = std::find_if(campaigns_.cbegin(), campaigns_.cend(), [&](const JoinedCampaign &entry)
    {
        return entry.toData().id == selectedId;
    });
    if (campaign == campaigns_.cend())
    {
        setErrorMessage(tr("La campagna selezionata non è più disponibile."));
        return false;
    }
    if (!campaignDirectory_.isEmpty())
    {
        const auto path = QDir(campaignDirectory_).absoluteFilePath(id + QStringLiteral(".json"));
        const QFileInfo file(path);
        if (file.isSymLink() || (file.exists() && (!file.isFile()
            || readCampaignData(path).value(QStringLiteral("token")).toString() != QString::fromStdString(campaign->toData().token.value())
            || !QFile::remove(path))))
        {
            setErrorMessage(tr("Impossibile rimuovere la campagna salvata su questo PC."));
            return false;
        }
    }
    campaigns_.removeAt(campaign - campaigns_.cbegin());
    emit campaignsChanged();
    return true;
}

void SessionController::openHostedCampaign(int campaignIndex)
{
    if (busy() || !activeName_.isEmpty() || campaignIndex < 0 || campaignIndex >= hostedCampaigns_.size()) return;
    clearError();
    auto data = hostedCampaigns_.at(campaignIndex).toData();
    try
    {
        const QDir directory(QDir(campaignDirectory_).filePath(QString::fromStdString(data.id)));
        data.code = openCampaignCode(directory);
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile caricare o salvare il codice d’invito della campagna."));
        return;
    }
    openHostedCampaign_ = campaignIndex;
    hostedCampaigns_[campaignIndex] = HostedCampaign(std::move(data));
    const auto view = hostedCampaigns_.at(campaignIndex).toView();
    activeName_ = QString::fromStdString(view.name);
    openInviteCode_ = normalizedCode(*view.code);
    emit hostedCampaignsChanged();
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

std::optional<std::string> SessionController::validatedDisplayName(const QString &displayName)
{
    try { return normalizeDisplayName(displayName.toStdString()); }
    catch (const std::invalid_argument &)
    {
        setErrorMessage(tr("Inserisci un nome visualizzato da 1 a 48 caratteri, su una sola riga."));
        return std::nullopt;
    }
}

void SessionController::startHosting(int port, const QString &displayName)
{
    if (!openHostedCampaign_ || hosting_ || busy()) return;
    clearError();
    if (port < 0 || port > 65535)
    {
        setErrorMessage(tr("Inserisci una porta valida."));
        return;
    }

    const auto name = validatedDisplayName(displayName);
    if (!name) return;
    try
    {
        hostingReply_ = std::async(std::launch::async, &SessionController::launchHosting,
            campaignDirectory_, hostedCampaigns_.at(*openHostedCampaign_).toData(), port, *name);
    }
    catch (const std::exception &)
    {
        setErrorMessage(tr("Impossibile avviare l’host."));
        return;
    }
    pollTimer_.start();
    emit busyChanged();
}

SessionController::Hosting SessionController::launchHosting(const QString &campaignDirectory, const HostedCampaignData &campaign, int port, const std::string &displayName)
{
    const QDir directory(QDir(campaignDirectory).filePath(QString::fromStdString(campaign.id)));
    Hosting result;
    result.lock = std::make_unique<QLockFile>(hostedLockPath(directory));
    result.lock->setStaleLockTime(0);
    if (!result.lock->tryLock())
    {
        result.error = result.lock->error() == QLockFile::LockFailedError
            ? tr("Questa campagna è già in hosting su questo PC.")
            : tr("Impossibile accedere alla cartella della campagna.");
        return result;
    }
    result.legacyLock = std::make_unique<QLockFile>(directory.filePath(QStringLiteral("hosting.lock")));
    result.legacyLock->setStaleLockTime(0);
    if (!result.legacyLock->tryLock())
    {
        result.error = tr("Questa campagna è già in uso o non è accessibile.");
        return result;
    }

    try
    {
        const auto manifest = directory.filePath(QStringLiteral("campaign.json"));
        const auto code = campaignCode(readCampaignData(manifest));
        result.session = std::make_unique<SessionServer>(campaign.id, code, directory.filesystemAbsolutePath());
        const auto archive = std::filesystem::u8path(directory.filePath(QStringLiteral("identities.json")).toStdString());
        const bool existingArchive = std::filesystem::exists(archive);
        if (existingArchive) result.session->loadIdentityState(archive);
        result.session->setHostDisplayName(displayName);
        if (!existingArchive) result.session->saveIdentityState(archive);
        result.session->registerSessionInfo(campaign.name);
    }
    catch (const std::exception &)
    {
        result.error = tr("Impossibile preparare il codice, le identità o l’elenco delle schede della campagna.");
    }
    if (result.error.isEmpty())
    {
        try
        {
            result.session->startHosting("0.0.0.0", static_cast<std::uint16_t>(port));
            result.port = *result.session->toView().listeningPort;
        }
        catch (const std::exception &)
        {
            result.error = tr("Impossibile avviare l’host su questa porta. Prova a sceglierne un’altra.");
        }
    }
    if (!result.error.isEmpty())
    {
        result.session.reset();
        result.legacyLock.reset();
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
            updateParticipants(hosting_->session->toView().participants, std::nullopt, true);
            pollTimer_.start();
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
    pollTimer_.stop();
    permissionsReply_ = {};
    clearError();
    try { hosting_->session->stopHosting(); }
    catch (const std::exception &) { setErrorMessage(tr("Si è verificato un errore durante l’arresto dell’host.")); }
    hosting_.reset();
    setParticipants({});
    emit hostingChanged();
    closeHostedCampaign();
}

void SessionController::enter(const QString &address, const QString &codeText, int campaignIndex, const QString &displayName)
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
    if (campaignIndex == -1)
    {
        for (int index = 0; index < campaigns_.size(); ++index)
        {
            const auto view = campaigns_.at(index).toView();
            if (view.address != endpoint.toStdString() || !view.code.matches(code->value())) continue;
            campaignIndex = index;
            break;
        }
    }

    const auto name = validatedDisplayName(displayName);
    if (!name) return;
    try
    {
        admissionReply_ = campaignIndex == -1
            ? session_.joinSession(ip.toStdString(), port, *code, *name)
            : session_.resumeSession(ip.toStdString(), port, *code, campaigns_.at(campaignIndex).toData().token, *name);
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
    pending_ = Admission{endpoint, *code, campaignIndex, std::nullopt};
    pollTimer_.start();
    emit busyChanged();
}

void SessionController::disconnectSession()
{
    if (activeName_.isEmpty() || openHostedCampaign_) return;
    pollTimer_.stop();
    permissionsReply_ = {};
    session_.disconnectFromHost();
    setParticipants({});
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
    if (hosting_)
    {
        updateParticipants(hosting_->session->toView().participants, std::nullopt, true);
        pollParticipantPermissions();
        return;
    }
    if (!pending_)
    {
        const auto client = session_.toView();
        if (client.state == ClientConnectionState::Disconnected)
        {
            setErrorMessage(tr("La connessione con l’host è stata interrotta."));
            disconnectSession();
        }
        else
        {
            if (client.participants)
                updateParticipants(*client.participants, client.userId, client.permissions.value_or(PermissionSet{}).hasAll(Permission::ChangePermissions));
            pollParticipantPermissions();
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
        campaigns_.append(JoinedCampaign(JoinedCampaignData{
            QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(), admission.address.toStdString(),
            admission.code, tr("Sessione %1").arg(admission.address).toStdString(), *admission.newToken
        }));
    }
    auto &campaign = campaigns_[index];
    campaign.updateConnection(admission.address.toStdString(), admission.code, name.toStdString());
    const bool saved = saveCampaign(campaign.toData());
    const auto client = session_.toView();

    if (client.state != ClientConnectionState::Connected)
    {
        emit campaignsChanged();
        failAdmission(tr("La connessione con l’host è stata interrotta."));
        return;
    }
    activeName_ = QString::fromStdString(campaign.toView().name);
    pending_.reset();
    if (client.participants)
        updateParticipants(*client.participants, client.userId, client.permissions.value_or(PermissionSet{}).hasAll(Permission::ChangePermissions));
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

void SessionController::updateParticipants(const ParticipantsSnapshot &snapshot, const std::optional<UserId> &localUser, bool canManagePermissions)
{
    QVariantList rows;
    for (const auto &participant : snapshot.participants)
    {
        const bool isSelf = hosting_ ? participant.isHost : localUser == participant.userId;
        rows.append(QVariantMap{
            {QStringLiteral("userId"), QString::fromStdString(participant.userId)},
            {QStringLiteral("displayName"), QString::fromStdString(participant.displayName)},
            {QStringLiteral("isHost"), participant.isHost},
            {QStringLiteral("isSelf"), isSelf},
            {QStringLiteral("canEditPermissions"), canManagePermissions && !isSelf && !participant.isHost}
        });
    }
    setParticipants(std::move(rows));
}

void SessionController::setParticipants(QVariantList participants)
{
    if (participants_ == participants) return;
    participants_ = std::move(participants);
    emit participantsChanged();
}

void SessionController::readParticipantPermissions(const QString &userId)
{
    requestParticipantPermissions({{"targetUserId", userId.toStdString()}}, false);
}

void SessionController::updateParticipantPermissions(const QString &userId, int grant, int revoke)
{
    requestParticipantPermissions({{"targetUserId", userId.toStdString()}, {"grant", grant}, {"revoke", revoke}}, true);
}

void SessionController::cancelParticipantPermissionRead()
{
    if (!savingPermissions_) permissionsReply_ = {};
}

void SessionController::requestParticipantPermissions(nlohmann::json payload, bool saving)
{
    if (permissionsReply_.valid()) return;
    if (!hosting_ && (activeName_.isEmpty() || openHostedCampaign_))
    {
        emit participantPermissionsFailed(tr("La sessione non è attiva."));
        return;
    }
    try
    {
        const auto command = saving ? "permissions.update" : "permissions.get";
        permissionsReply_ = hosting_ ? hosting_->session->sendCommand(command, std::move(payload)) : session_.sendCommand(command, std::move(payload));
        savingPermissions_ = saving;
    }
    catch (const std::exception &)
    {
        emit participantPermissionsFailed(tr("Impossibile inviare la richiesta all’host."));
    }
}

void SessionController::pollParticipantPermissions()
{
    using namespace std::chrono_literals;
    if (!permissionsReply_.valid() || permissionsReply_.wait_for(0ms) != std::future_status::ready) return;
    const bool saving = std::exchange(savingPermissions_, false);
    try
    {
        const auto response = permissionsReply_.get();
        const auto &value = response.at("permissions");
        if (!value.is_number_integer()) throw std::invalid_argument("invalid permission response");
        const auto mask = PermissionSet::fromMask(value.get<std::uint64_t>()).toMask();
        if (saving) emit participantPermissionsSaved();
        else emit participantPermissionsLoaded(static_cast<int>(mask));
        return;
    }
    catch (const ProtocolError &error)
    {
        if (error.code() == ProtocolErrorCode::Forbidden)
        {
            emit participantPermissionsFailed(tr("Non hai accesso alla gestione dei permessi di questa persona."));
            return;
        }
        if (saving && error.code() == ProtocolErrorCode::UnknownCommand)
        {
            emit participantPermissionsFailed(tr("L’app dell’host deve essere aggiornata per salvare i permessi."));
            return;
        }
    }
    catch (const std::exception &) {}
    emit participantPermissionsFailed(saving ? tr("Impossibile salvare i permessi.") : tr("Impossibile leggere i permessi."));
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
            campaigns_.append(JoinedCampaign(JoinedCampaignData{
                entry.completeBaseName().toStdString(), address.toStdString(), code, name.toStdString(), token
            }));
        }
        catch (const std::invalid_argument &) {} // Keep unreadable entries on disk.
    }

    for (const auto &entry : directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name))
    {
        const auto id = entry.fileName();
        if (QUuid(id).toString(QUuid::WithoutBraces) != id) continue;
        const auto data = readCampaignData(QDir(entry.filePath()).filePath(QStringLiteral("campaign.json")));
        const auto name = data.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty()) continue;
        try
        {
            const auto code = data.contains(QStringLiteral("code")) ? std::make_optional(campaignCode(data)) : std::nullopt;
            hostedCampaigns_.append(HostedCampaign(HostedCampaignData{id.toStdString(), name.toStdString(), code}));
        }
        catch (const std::invalid_argument &) {} // Keep unreadable entries on disk.
    }
}

bool SessionController::saveCampaign(const JoinedCampaignData &campaign) const
{
    if (campaignDirectory_.isEmpty() || !QDir().mkpath(campaignDirectory_)) return false;
    const QJsonObject data{
        {QStringLiteral("address"), QString::fromStdString(campaign.address)},
        {QStringLiteral("code"), normalizedCode(campaign.code)},
        {QStringLiteral("name"), QString::fromStdString(campaign.name)},
        {QStringLiteral("token"), QString::fromStdString(campaign.token.value())}
    };
    return writeCampaignData(QDir(campaignDirectory_).filePath(QString::fromStdString(campaign.id) + QStringLiteral(".json")), data);
}
