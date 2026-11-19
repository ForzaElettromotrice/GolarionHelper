#include "../session_controller.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTest>

#include <chrono>
#include <filesystem>
#include <mutex>
#include <vector>

using namespace golarion::play;

class SessionControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void restoresMembershipAfterRestart();
    void listsMultipleCampaigns();
    void reopensSavedCampaignOffline();
    void hostsAndRestoresPlayer_data();
    void hostsAndRestoresPlayer();
};

void SessionControllerTest::restoresMembershipAfterRestart()
{
    QTemporaryDir data;
    QVERIFY(data.isValid());
    const auto campaignDirectory = data.filePath(QStringLiteral("campaigns"));
    const auto archive = std::filesystem::u8path(data.filePath(QStringLiteral("host.json")).toUtf8().constData());
    constexpr auto name = "La Città Sommersa";
    std::mutex observedMutex;
    std::vector<UserId> observedUsers;
    const auto registerName = [&](Session &host)
    {
        host.add_command("session.info", CommandRequirements{}, [&](const CommandContext &context, const nlohmann::json &)
        {
            const std::lock_guard lock(observedMutex);
            observedUsers.push_back(context.userId.value());
            return nlohmann::json{{"name", name}};
        });
    };

    {
        auto host = Session::createHost("host");
        host.saveIdentityState(archive);
        registerName(host);
        const auto hostAddress = QStringLiteral("127.0.0.1:%1").arg(host.startHosting("127.0.0.1", 0));
        const auto code = QString::fromStdString(host.getSessionCode());
        SessionController controller(campaignDirectory);
        QVERIFY(controller.campaigns().isEmpty());

        controller.enter(hostAddress, code == QStringLiteral("AAAAA") ? QStringLiteral("BBBBB") : QStringLiteral("AAAAA"), -1);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY(!controller.errorMessage().isEmpty());
        QVERIFY(controller.activeName().isEmpty());
        QVERIFY(controller.campaigns().isEmpty());

        controller.enter(hostAddress, code.toLower(), -1);
        QTRY_COMPARE_WITH_TIMEOUT(controller.activeName(), QString::fromUtf8(name), 5000);
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(controller.campaigns().size(), 1);
        QVERIFY(!controller.campaigns().front().toMap().contains(QStringLiteral("token")));
        QCOMPARE(QDir(campaignDirectory).entryList(QDir::Files).size(), 1);

        controller.disconnectSession();
        QVERIFY(controller.activeName().isEmpty());
        QCOMPARE(controller.campaigns().size(), 1);
    }

    auto reopenedHost = Session::createHost("host");
    reopenedHost.loadIdentityState(archive);
    registerName(reopenedHost);
    const auto newAddress = QStringLiteral("127.0.0.1:%1").arg(reopenedHost.startHosting("127.0.0.1", 0));
    SessionController reopened(campaignDirectory);
    QCOMPARE(reopened.campaigns().size(), 1);
    reopened.enter(newAddress, QString::fromStdString(reopenedHost.getSessionCode()), 0);
    QTRY_COMPARE_WITH_TIMEOUT(reopened.activeName(), QString::fromUtf8(name), 5000);
    QCOMPARE(reopened.campaigns().size(), 1);
    QCOMPARE(reopened.campaigns().front().toMap().value(QStringLiteral("address")).toString(), newAddress);

    reopenedHost.stopHosting();
    QTRY_VERIFY_WITH_TIMEOUT(reopened.activeName().isEmpty(), 5000);
    QVERIFY(!reopened.errorMessage().isEmpty());
    QCOMPARE(reopened.campaigns().size(), 1);
    const std::lock_guard lock(observedMutex);
    QCOMPARE(observedUsers.size(), 2);
    QCOMPARE(observedUsers[0], observedUsers[1]);
}

void SessionControllerTest::listsMultipleCampaigns()
{
    QTemporaryDir data;
    QVERIFY(data.isValid());
    const auto archiveDirectory = std::filesystem::u8path(data.path().toStdString());
    auto firstHost = Session::createHost("host");
    auto secondHost = Session::createHost("host");
    firstHost.saveIdentityState(archiveDirectory / "first-host.json");
    secondHost.saveIdentityState(archiveDirectory / "second-host.json");
    const auto firstAddress = QStringLiteral("127.0.0.1:%1").arg(firstHost.startHosting("127.0.0.1", 0));
    const auto secondAddress = QStringLiteral("127.0.0.1:%1").arg(secondHost.startHosting("127.0.0.1", 0));
    const auto firstCode = QString::fromStdString(firstHost.getSessionCode());
    const auto secondCode = QString::fromStdString(secondHost.getSessionCode());
    SessionController controller(data.filePath(QStringLiteral("campaigns")));

    controller.enter(firstAddress, firstCode, -1);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
    QVERIFY2(controller.activeName().contains(firstAddress), qPrintable(controller.errorMessage()));
    controller.disconnectSession();

    controller.enter(secondAddress, secondCode, -1);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
    QVERIFY2(controller.activeName().contains(secondAddress), qPrintable(controller.errorMessage()));
    controller.disconnectSession();
    QCOMPARE(controller.campaigns().size(), 2);

    controller.enter(firstAddress, firstCode, -1);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
    QVERIFY2(controller.activeName().contains(firstAddress), qPrintable(controller.errorMessage()));
    QCOMPARE(controller.campaigns().size(), 2);
}

void SessionControllerTest::reopensSavedCampaignOffline()
{
    QTemporaryDir data;
    QVERIFY(data.isValid());
    const auto name = QStringLiteral("La città di vetro");
    {
        SessionController controller(data.path());
        QVERIFY(!controller.createHostedCampaign(QStringLiteral("   ")));
        QVERIFY(!controller.errorMessage().isEmpty());
        QVERIFY(controller.hostedCampaigns().isEmpty());
        QVERIFY(controller.createHostedCampaign(QStringLiteral("  ") + name + QStringLiteral("  ")));
        QCOMPARE(controller.hostedCampaigns(), QStringList{name});
        QCOMPARE(controller.hostingPort(), 0);
        QVERIFY(controller.inviteCode().isEmpty());
    }

    SessionController reopened(data.path());
    QCOMPARE(reopened.hostedCampaigns(), QStringList{name});
    QVERIFY(reopened.campaigns().isEmpty());
    reopened.openHostedCampaign(0);
    QVERIFY(reopened.hostedCampaignOpen());
    QCOMPARE(reopened.inviteCode().size(), 5);
    QCOMPARE(reopened.activeName(), name);
    QCOMPARE(reopened.hostingPort(), 0);
    reopened.closeHostedCampaign();
    QVERIFY(!reopened.hostedCampaignOpen());
    QVERIFY(reopened.activeName().isEmpty());
    QCOMPARE(reopened.hostedCampaigns(), QStringList{name});
}

void SessionControllerTest::hostsAndRestoresPlayer_data()
{
    QTest::addColumn<bool>("legacyCampaign");
    QTest::newRow("new campaign") << false;
    QTest::newRow("legacy campaign without code") << true;
}

void SessionControllerTest::hostsAndRestoresPlayer()
{
    QFETCH(bool, legacyCampaign);
    using namespace std::chrono_literals;
    QTemporaryDir data;
    QVERIFY(data.isValid());
    const auto name = QStringLiteral("La città di vetro");
    auto player = Session::createClient();
    std::optional<ClientAdmission> joined;
    QString firstCode;
    {
        SessionController controller(data.path());
        QVERIFY(controller.createHostedCampaign(name));
        const QDir directory(data.path());
        const auto folders = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        QCOMPARE(folders.size(), 1);
        QFile manifest(directory.filePath(folders.front() + QStringLiteral("/campaign.json")));
        QVERIFY(manifest.open(QIODevice::ReadOnly));
        auto record = QJsonDocument::fromJson(manifest.readAll()).object();
        manifest.close();
        firstCode = record.value(QStringLiteral("code")).toString();
        QCOMPARE(firstCode.size(), 5);
        if (legacyCampaign)
        {
            record.remove(QStringLiteral("code"));
            QVERIFY(manifest.open(QIODevice::WriteOnly | QIODevice::Truncate));
            const auto document = QJsonDocument(record).toJson();
            QCOMPARE(manifest.write(document), document.size());
            manifest.close();
        }
        controller.openHostedCampaign(0);
        if (legacyCampaign) firstCode = controller.inviteCode();
        QCOMPARE(controller.inviteCode(), firstCode);
        QCOMPARE(firstCode.size(), 5);

        QTcpServer blocker;
        QVERIFY(blocker.listen(QHostAddress::AnyIPv4));
        controller.startHosting(blocker.serverPort());
        QVERIFY(controller.busy());
        QCOMPARE(controller.hostingPort(), 0);
        controller.closeHostedCampaign();
        QCOMPARE(controller.activeName(), name);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(controller.activeName(), name);
        blocker.close();

        controller.startHosting(0);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY2(controller.hostingPort() > 0, qPrintable(controller.errorMessage()));
        QVERIFY(!controller.inviteEndpoints().isEmpty());
        QVERIFY(controller.inviteEndpoints().front().endsWith(QStringLiteral(":%1").arg(controller.hostingPort())));
        QCOMPARE(controller.inviteCode(), firstCode);
        const SessionCode code(firstCode.toStdString());

        SessionController otherWindow(data.path());
        otherWindow.openHostedCampaign(0);
        otherWindow.startHosting(0);
        QTRY_VERIFY_WITH_TIMEOUT(!otherWindow.busy(), 5000);
        QCOMPARE(otherWindow.hostingPort(), 0);
        QVERIFY(!otherWindow.errorMessage().isEmpty());

        auto join = player.joinSession("127.0.0.1", static_cast<std::uint16_t>(controller.hostingPort()), code);
        QCOMPARE(join.wait_for(5s), std::future_status::ready);
        joined = join.get();
        QVERIFY(joined->userToken);

        auto info = player.sendCommand("session.info", nlohmann::json::object());
        QCOMPARE(info.wait_for(5s), std::future_status::ready);
        QCOMPARE(QString::fromStdString(info.get().at("name").get<std::string>()), name);

        controller.stopHosting();
        QTRY_VERIFY_WITH_TIMEOUT(player.getClientState().state == ClientConnectionState::Disconnected, 5000);
        QCOMPARE(controller.hostingPort(), 0);
        QVERIFY(controller.inviteCode().isEmpty());
        QVERIFY(!controller.hostedCampaignOpen());
        QCOMPARE(controller.hostedCampaigns(), QStringList{name});
    }

    SessionController reopened(data.path());
    reopened.openHostedCampaign(0);
    QCOMPARE(reopened.inviteCode(), firstCode);
    reopened.startHosting(0);
    QTRY_VERIFY_WITH_TIMEOUT(!reopened.busy(), 5000);
    QVERIFY2(reopened.hostingPort() > 0, qPrintable(reopened.errorMessage()));
    QCOMPARE(reopened.inviteCode(), firstCode);
    const SessionCode code(firstCode.toStdString());
    auto resume = player.resumeSession("127.0.0.1", static_cast<std::uint16_t>(reopened.hostingPort()), code, *joined->userToken);
    QCOMPARE(resume.wait_for(5s), std::future_status::ready);
    const auto restored = resume.get();
    QCOMPARE(restored.userId, joined->userId);
    QVERIFY(!restored.userToken);
}

QTEST_GUILESS_MAIN(SessionControllerTest)
#include "session_controller_test.moc"
