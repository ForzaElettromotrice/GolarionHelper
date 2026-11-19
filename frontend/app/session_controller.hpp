#pragma once

#include "golarion/play/session.hpp"

#include <QLockFile>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <future>
#include <memory>
#include <optional>

class SessionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Provided by the application")
    Q_PROPERTY(QVariantList campaigns READ campaigns NOTIFY campaignsChanged FINAL)
    Q_PROPERTY(QStringList hostedCampaigns READ hostedCampaigns NOTIFY hostedCampaignsChanged FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString activeName READ activeName NOTIFY activeNameChanged FINAL)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged FINAL)
    Q_PROPERTY(bool hostedCampaignOpen READ hostedCampaignOpen NOTIFY activeNameChanged FINAL)
    Q_PROPERTY(int hostingPort READ hostingPort NOTIFY hostingChanged FINAL)
    Q_PROPERTY(QStringList inviteEndpoints READ inviteEndpoints NOTIFY hostingChanged FINAL)
    Q_PROPERTY(QString inviteCode READ inviteCode NOTIFY activeNameChanged FINAL)

public:
    explicit SessionController(QString campaignDirectory);

    QVariantList campaigns() const;
    QStringList hostedCampaigns() const;
    bool busy() const;
    QString activeName() const;
    QString errorMessage() const;
    bool hostedCampaignOpen() const;
    int hostingPort() const;
    QStringList inviteEndpoints() const;
    QString inviteCode() const;

    Q_INVOKABLE void enter(const QString &address, const QString &code, int campaignIndex);
    Q_INVOKABLE void disconnectSession();
    Q_INVOKABLE void clearError();
    Q_INVOKABLE bool createHostedCampaign(const QString &name);
    Q_INVOKABLE void openHostedCampaign(int campaignIndex);
    Q_INVOKABLE void closeHostedCampaign();
    Q_INVOKABLE void startHosting(int port);
    Q_INVOKABLE void stopHosting();
    Q_INVOKABLE void copyInvitationText(const QString &text);

signals:
    void campaignsChanged();
    void hostedCampaignsChanged();
    void busyChanged();
    void activeNameChanged();
    void hostingChanged();
    void errorMessageChanged();
    void sessionOpened();
    void sessionClosed();

private:
    struct Campaign
    {
        QString id;
        QString address;
        QString code;
        QString name;
        golarion::play::UserToken token;
    };

    struct Admission
    {
        QString address;
        QString code;
        int campaignIndex;
        std::optional<golarion::play::UserToken> newToken;
    };

    struct HostedCampaign
    {
        QString id;
        QString name;
    };

    struct Hosting
    {
        // Shut down the session before releasing the archive lock.
        std::unique_ptr<QLockFile> lock;
        std::unique_ptr<golarion::play::Session> session;
        QStringList endpoints;
        QString error;
        int port = 0;
    };

    static Hosting launchHosting(const QString &campaignDirectory, const HostedCampaign &campaign, int port);
    void pollHosting();
    void loadCampaigns();
    bool saveCampaign(const Campaign &campaign) const;
    void pollSession();
    void finishAdmission(const QString &name);
    void failAdmission(const QString &message);
    void setErrorMessage(const QString &message);

    const QString campaignDirectory_;
    QList<Campaign> campaigns_;
    QList<HostedCampaign> hostedCampaigns_;
    QString activeName_;
    QString errorMessage_;
    std::optional<Admission> pending_;
    std::future<golarion::play::ClientAdmission> admissionReply_;
    std::future<nlohmann::json> infoReply_;
    golarion::play::Session session_ = golarion::play::Session::createClient();
    QTimer pollTimer_;
    std::optional<int> openHostedCampaign_;
    QString openInviteCode_;
    std::optional<Hosting> hosting_;
    std::future<Hosting> hostingReply_;
};
