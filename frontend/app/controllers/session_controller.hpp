#pragma once

#include "golarion/play/session_client.hpp"
#include "golarion/play/session_server.hpp"
#include "golarion/play/campaign.hpp"

#include <QLockFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <future>
#include <functional>
#include <memory>
#include <optional>

class SessionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QVariantList campaigns READ campaigns NOTIFY campaignsChanged FINAL)
    Q_PROPERTY(QVariantList hostedCampaigns READ hostedCampaigns NOTIFY hostedCampaignsChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString activeName READ activeName NOTIFY activeNameChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(bool hostedCampaignOpen READ hostedCampaignOpen NOTIFY activeNameChanged FINAL)
    Q_PROPERTY(int hostingPort READ hostingPort NOTIFY hostingChanged FINAL)
    Q_PROPERTY(QStringList inviteEndpoints READ inviteEndpoints NOTIFY hostingChanged FINAL)
    Q_PROPERTY(QString inviteCode READ inviteCode NOTIFY activeNameChanged FINAL)
    Q_PROPERTY(QVariantList participants READ participants NOTIFY participantsChanged FINAL)

public:
    explicit SessionController(QString campaignDirectory, std::function<bool(const QString &)> moveToTrash = {});

    QVariantList campaigns() const;
    QVariantList hostedCampaigns() const;
    bool busy() const;
    QString activeName() const;
    QString errorMessage() const;
    bool hostedCampaignOpen() const;
    int hostingPort() const;
    QStringList inviteEndpoints() const;
    QString inviteCode() const;
    QVariantList participants() const;

    Q_INVOKABLE void enter(const QString &address, const QString &code, int campaignIndex, const QString &displayName);
    Q_INVOKABLE void disconnectSession();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE bool createHostedCampaign(const QString &name);
    Q_INVOKABLE bool copyHostedCampaignName(const QString &id);
    Q_INVOKABLE bool trashHostedCampaign(const QString &id, const QString &confirmationName, const QString &code);
    Q_INVOKABLE bool forgetJoinedCampaign(const QString &id);
    Q_INVOKABLE void openHostedCampaign(int campaignIndex);
    Q_INVOKABLE void closeHostedCampaign();
    Q_INVOKABLE void startHosting(int port, const QString &displayName);
    Q_INVOKABLE void stopHosting();
    Q_INVOKABLE void copyInvitationText(const QString &text);
    Q_INVOKABLE void readParticipantPermissions(const QString &userId);
    Q_INVOKABLE void updateParticipantPermissions(const QString &userId, int grant, int revoke);
    Q_INVOKABLE void cancelParticipantPermissionRead();

signals:
    void campaignsChanged();
    void hostedCampaignsChanged();
    void busyChanged();
    void activeNameChanged();
    void hostingChanged();
    void participantsChanged();
    void errorMessageChanged();
    void sessionOpened();
    void sessionClosed();
    void participantPermissionsLoaded(int mask);
    void participantPermissionsSaved();
    void participantPermissionsFailed(const QString &message);

private:
    struct Admission
    {
        QString address;
        golarion::play::SessionCode code;
        int campaignIndex;
        std::optional<golarion::play::UserToken> newToken;
    };

    struct Hosting
    {
        // Shut down the session before releasing the archive lock.
        std::unique_ptr<QLockFile> lock;
        std::unique_ptr<QLockFile> legacyLock;
        std::unique_ptr<golarion::play::SessionServer> session;
        QStringList endpoints;
        QString error;
        int port = 0;
    };

    static Hosting launchHosting(golarion::play::SessionClient &client, const QString &campaignDirectory, const golarion::play::HostedCampaignData &campaign, int port, const std::string &displayName);
    std::optional<std::string> validatedDisplayName(const QString &displayName);
    void pollHosting();
    void loadCampaigns();
    bool saveCampaign(const golarion::play::JoinedCampaignData &data) const;
    void pollSession();
    void finishAdmission(const QString &name);
    void failAdmission(const QString &message);
    void setErrorMessage(const QString &message);
    void updateParticipants(const golarion::play::ParticipantsSnapshot &snapshot, const std::optional<golarion::play::UserId> &localUser, bool canManagePermissions);
    void setParticipants(QVariantList participants);
    void requestParticipantPermissions(nlohmann::json payload, bool saving);
    void pollParticipantPermissions();

    const QString campaignDirectory_;
    const std::function<bool(const QString &)> moveToTrash_;
    QList<golarion::play::JoinedCampaign> campaigns_;
    QList<golarion::play::HostedCampaign> hostedCampaigns_;
    QString activeName_;
    QString errorMessage_;
    QVariantList participants_;
    std::optional<Admission> pending_;
    std::future<golarion::play::ClientAdmission> admissionReply_;
    std::future<nlohmann::json> infoReply_;
    std::future<nlohmann::json> permissionsReply_;
    bool savingPermissions_ = false;
    golarion::play::SessionClient session_;
    QTimer pollTimer_;
    std::optional<int> openHostedCampaign_;
    QString openInviteCode_;
    std::optional<Hosting> hosting_;
    std::future<Hosting> hostingReply_;
};
