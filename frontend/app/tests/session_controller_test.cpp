#include "../session_controller.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QtQml/qqml.h>
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
    void removesOnlySelectedHostedCampaign();
    void forgetsOnlySelectedMembership();
    void requiresExplicitConfirmationInCampaignList();
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

        controller.enter(hostAddress, code == QStringLiteral("AAAAA") ? QStringLiteral("BBBBB") : QStringLiteral("AAAAA"), -1, QStringLiteral("Giulia"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY(!controller.errorMessage().isEmpty());
        QVERIFY(controller.activeName().isEmpty());
        QVERIFY(controller.campaigns().isEmpty());

        controller.enter(hostAddress, code.toLower(), -1, QStringLiteral("Giulia"));
        QTRY_COMPARE_WITH_TIMEOUT(controller.activeName(), QString::fromUtf8(name), 5000);
        QTRY_COMPARE_WITH_TIMEOUT(controller.participants().size(), 2, 5000);
        const auto participants = controller.participants();
        QVERIFY(participants.front().toMap().value(QStringLiteral("isHost")).toBool());
        QCOMPARE(participants.front().toMap().value(QStringLiteral("displayName")).toString(), QStringLiteral("Host"));
        QVERIFY(!participants.front().toMap().value(QStringLiteral("isSelf")).toBool());
        QVERIFY(participants.back().toMap().value(QStringLiteral("isSelf")).toBool());
        QCOMPARE(participants.back().toMap().value(QStringLiteral("displayName")).toString(), QStringLiteral("Giulia"));
        QVERIFY2(controller.errorMessage().isEmpty(), qPrintable(controller.errorMessage()));
        QCOMPARE(controller.campaigns().size(), 1);
        QVERIFY(!controller.campaigns().front().toMap().contains(QStringLiteral("token")));
        QVERIFY(!controller.forgetJoinedCampaign(controller.campaigns().front().toMap().value(QStringLiteral("id")).toString()));
        QCOMPARE(controller.campaigns().size(), 1);
        QCOMPARE(QDir(campaignDirectory).entryList(QDir::Files).size(), 1);

        controller.disconnectSession();
        QVERIFY(controller.activeName().isEmpty());
        QVERIFY(controller.participants().isEmpty());
        QCOMPARE(controller.campaigns().size(), 1);
    }

    auto reopenedHost = Session::createHost("host");
    reopenedHost.loadIdentityState(archive);
    registerName(reopenedHost);
    const auto newAddress = QStringLiteral("127.0.0.1:%1").arg(reopenedHost.startHosting("127.0.0.1", 0));
    SessionController reopened(campaignDirectory);
    QCOMPARE(reopened.campaigns().size(), 1);
    reopened.enter(newAddress, QString::fromStdString(reopenedHost.getSessionCode()), 0, QStringLiteral("Giulia R."));
    QTRY_COMPARE_WITH_TIMEOUT(reopened.activeName(), QString::fromUtf8(name), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(reopened.participants().size(), 2, 5000);
    QCOMPARE(reopened.participants().back().toMap().value(QStringLiteral("displayName")).toString(), QStringLiteral("Giulia R."));
    QCOMPARE(reopened.campaigns().size(), 1);
    QCOMPARE(reopened.campaigns().front().toMap().value(QStringLiteral("address")).toString(), newAddress);

    reopenedHost.stopHosting();
    QTRY_VERIFY_WITH_TIMEOUT(reopened.activeName().isEmpty(), 5000);
    QVERIFY(reopened.participants().isEmpty());
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

    controller.enter(firstAddress, firstCode, -1, QStringLiteral("Giulia"));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
    QVERIFY2(controller.activeName().contains(firstAddress), qPrintable(controller.errorMessage()));
    controller.disconnectSession();

    controller.enter(secondAddress, secondCode, -1, QStringLiteral("Giulia"));
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
    QVERIFY2(controller.activeName().contains(secondAddress), qPrintable(controller.errorMessage()));
    controller.disconnectSession();
    QCOMPARE(controller.campaigns().size(), 2);

    controller.enter(firstAddress, firstCode, -1, QStringLiteral("Giulia"));
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
        QCOMPARE(controller.hostedCampaigns().size(), 1);
        QCOMPARE(controller.hostedCampaigns().front().toMap().value(QStringLiteral("name")).toString(), name);
        QCOMPARE(controller.hostingPort(), 0);
        QVERIFY(controller.inviteCode().isEmpty());
    }

    SessionController reopened(data.path());
    QCOMPARE(reopened.hostedCampaigns().size(), 1);
    QCOMPARE(reopened.hostedCampaigns().front().toMap().value(QStringLiteral("name")).toString(), name);
    QVERIFY(reopened.campaigns().isEmpty());
    reopened.openHostedCampaign(0);
    QVERIFY(reopened.hostedCampaignOpen());
    QCOMPARE(reopened.inviteCode().size(), 5);
    QCOMPARE(reopened.activeName(), name);
    QCOMPARE(reopened.hostingPort(), 0);
    reopened.closeHostedCampaign();
    QVERIFY(!reopened.hostedCampaignOpen());
    QVERIFY(reopened.activeName().isEmpty());
    QCOMPARE(reopened.hostedCampaigns().size(), 1);
    QCOMPARE(reopened.hostedCampaigns().front().toMap().value(QStringLiteral("name")).toString(), name);
}

void SessionControllerTest::removesOnlySelectedHostedCampaign()
{
    QTemporaryDir data;
    QTemporaryDir trash;
    QVERIFY(data.isValid() && trash.isValid());
    bool acceptsMove = false;
    int attempts = 0;
    SessionController controller(data.path(), [&](const QString &path)
    {
        ++attempts;
        return acceptsMove && QDir().rename(path, trash.filePath(QFileInfo(path).fileName()));
    });
    const auto name = QStringLiteral("Città di vetro");
    QVERIFY(controller.createHostedCampaign(name));
    QVERIFY(controller.createHostedCampaign(name));
    const auto selected = controller.hostedCampaigns().at(1).toMap();
    const auto id = selected.value(QStringLiteral("id")).toString();
    const auto code = selected.value(QStringLiteral("code")).toString();
    const auto survivor = controller.hostedCampaigns().at(0).toMap().value(QStringLiteral("id")).toString();
    const auto path = data.filePath(id);
    QFile archive(QDir(path).filePath(QStringLiteral("identities.json")));
    QVERIFY(archive.open(QIODevice::WriteOnly));
    QCOMPARE(archive.write("identity archive"), 16);
    archive.close();

    QVERIFY(!controller.trashHostedCampaign(id, name.toLower(), code));
    QVERIFY(!controller.trashHostedCampaign(id, name, QStringLiteral("bad")));
    controller.openHostedCampaign(1);
    QVERIFY(controller.hostedCampaignOpen());
    QVERIFY(!controller.trashHostedCampaign(id, name, code));
    controller.closeHostedCampaign();

    for (const auto &lockPath : {path + QStringLiteral(".hosting.lock"), QDir(path).filePath(QStringLiteral("hosting.lock"))})
    {
        QLockFile lock(lockPath);
        QVERIFY(lock.tryLock());
        QVERIFY(!controller.trashHostedCampaign(id, name, code));
    }
    QCOMPARE(attempts, 0);

    QVERIFY(!controller.trashHostedCampaign(id, name, code));
    QCOMPARE(attempts, 1);
    QCOMPARE(controller.hostedCampaigns().size(), 2);
    QVERIFY(archive.exists());

    acceptsMove = true;
    QVERIFY2(controller.trashHostedCampaign(id, name, code), qPrintable(controller.errorMessage()));
    QCOMPARE(attempts, 2);
    QVERIFY(!QFileInfo::exists(path));
    QVERIFY(QFileInfo::exists(trash.filePath(id + QStringLiteral("/campaign.json"))));
    QVERIFY(QFileInfo::exists(trash.filePath(id + QStringLiteral("/identities.json"))));
    QVERIFY(QFileInfo::exists(data.filePath(survivor + QStringLiteral("/campaign.json"))));
    SessionController reopened(data.path());
    QCOMPARE(reopened.hostedCampaigns().size(), 1);
    QCOMPARE(reopened.hostedCampaigns().front().toMap().value(QStringLiteral("id")).toString(), survivor);
}

void SessionControllerTest::forgetsOnlySelectedMembership()
{
    QTemporaryDir data;
    QVERIFY(data.isValid());
    const auto first = data.filePath(QStringLiteral("first.json"));
    const auto second = data.filePath(QStringLiteral("second.json"));
    const auto writeMembership = [](const QString &path, const QString &token)
    {
        QFile file(path);
        const auto bytes = QJsonDocument(QJsonObject{
            {QStringLiteral("name"), QStringLiteral("Campagna")},
            {QStringLiteral("address"), QStringLiteral("127.0.0.1:42500")},
            {QStringLiteral("code"), QStringLiteral("A7K2Q")},
            {QStringLiteral("token"), token}
        }).toJson();
        return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
    };
    QVERIFY(writeMembership(first, QStringLiteral("0123456789abcdef")));
    QVERIFY(writeMembership(second, QStringLiteral("fedcba9876543210")));
    SessionController controller(data.path());
    QCOMPARE(controller.campaigns().size(), 2);
    QVERIFY(!controller.forgetJoinedCampaign(QStringLiteral("../outside")));
    QVERIFY(writeMembership(second, QStringLiteral("0000000000000000")));
    QVERIFY(!controller.forgetJoinedCampaign(QStringLiteral("second")));
    QCOMPARE(controller.campaigns().size(), 2);
    QVERIFY(QFileInfo::exists(second));

    QVERIFY(controller.forgetJoinedCampaign(QStringLiteral("first")));
    QCOMPARE(controller.campaigns().size(), 1);
    QVERIFY(!QFileInfo::exists(first));
    SessionController reopened(data.path());
    QCOMPARE(reopened.campaigns().size(), 1);
    QCOMPARE(reopened.campaigns().front().toMap().value(QStringLiteral("id")).toString(), QStringLiteral("second"));
}

void SessionControllerTest::requiresExplicitConfirmationInCampaignList()
{
    QTemporaryDir data;
    QTemporaryDir trash;
    QVERIFY(data.isValid() && trash.isValid());
    QFile membership(data.filePath(QStringLiteral("joined.json")));
    QVERIFY(membership.open(QIODevice::WriteOnly));
    QVERIFY(membership.write(R"({"name":"Remota","address":"127.0.0.1:42500","code":"A7K2Q","token":"0123456789abcdef"})") > 0);
    membership.close();
    SessionController controller(data.path(), [&](const QString &path)
    {
        return QDir().rename(path, trash.filePath(QFileInfo(path).fileName()));
    });
    QVERIFY(controller.createHostedCampaign(QStringLiteral("La campagna")));
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    qmlRegisterUncreatableType<SessionController>("Golarion.App", 1, 0, "SessionController", "Provided by the test");
    const auto page = QFINDTESTDATA("../CampaignsPage.qml");
    QVERIFY(!page.isEmpty());
    QQuickView view;
    view.setInitialProperties({{QStringLiteral("controller"), QVariant::fromValue(&controller)}, {QStringLiteral("localName"), QStringLiteral("Marta")}});
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(560, 440);
    view.setSource(QUrl::fromLocalFile(page));
    QVERIFY(view.rootObject());
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    const auto findItem = [&](const QString &name) -> QQuickItem *
    {
        auto items = view.contentItem()->childItems();
        while (!items.isEmpty())
        {
            auto *item = items.takeLast();
            if (item->objectName() == name) return item;
            items.append(item->childItems());
        }
        return nullptr;
    };
    auto *dialog = view.rootObject()->findChild<QObject *>(QStringLiteral("campaignRemovalDialog"));
    auto *hostedButton = findItem(QStringLiteral("trashHostedCampaignButton"));
    QVERIFY(dialog);
    QVERIFY(hostedButton);
    const auto click = [&](QQuickItem *button)
    {
        const auto center = QPointF(button->width() / 2, button->height() / 2);
        QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, button->mapToScene(center).toPoint());
    };

    click(hostedButton);
    QTRY_VERIFY(dialog->property("opened").toBool());
    auto *confirm = findItem(QStringLiteral("confirmCampaignRemoval"));
    auto *cancel = findItem(QStringLiteral("cancelCampaignRemoval"));
    auto *name = findItem(QStringLiteral("campaignNameConfirmation"));
    QVERIFY(confirm && cancel && name);
    QTRY_VERIFY(dialog->property("opened").toBool() && cancel->hasActiveFocus());
    QVERIFY(!confirm->isEnabled());
    name->setProperty("text", QStringLiteral("Nome diverso"));
    QVERIFY(!confirm->isEnabled());
    QVERIFY(!controller.hostedCampaignOpen());
    click(cancel);
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QCOMPARE(controller.hostedCampaigns().size(), 1);

    click(hostedButton);
    QTRY_VERIFY(dialog->property("opened").toBool());
    name->setProperty("text", QStringLiteral("La campagna"));
    QVERIFY(confirm->isEnabled());
    click(confirm);
    QTRY_VERIFY(controller.hostedCampaigns().isEmpty() && !dialog->property("visible").toBool());

    auto *joinedButton = findItem(QStringLiteral("forgetJoinedCampaignButton"));
    QVERIFY(joinedButton);
    click(joinedButton);
    QTRY_VERIFY(dialog->property("opened").toBool() && cancel->hasActiveFocus());
    QVERIFY(!name->isVisible() && confirm->isEnabled());
    QTest::keyClick(&view, Qt::Key_Escape);
    QTRY_VERIFY(!dialog->property("visible").toBool());
    QVERIFY(membership.exists());
    click(joinedButton);
    QTRY_VERIFY(dialog->property("opened").toBool());
    click(confirm);
    QTRY_VERIFY(controller.campaigns().isEmpty());
    QVERIFY(!membership.exists());
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
        controller.startHosting(blocker.serverPort(), QStringLiteral("Marta"));
        QVERIFY(controller.busy());
        QCOMPARE(controller.hostingPort(), 0);
        controller.closeHostedCampaign();
        QCOMPARE(controller.activeName(), name);
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY(!controller.errorMessage().isEmpty());
        QCOMPARE(controller.activeName(), name);
        blocker.close();

        controller.startHosting(0, QStringLiteral("Marta"));
        QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 5000);
        QVERIFY2(controller.hostingPort() > 0, qPrintable(controller.errorMessage()));
        QCOMPARE(controller.participants().size(), 1);
        const auto localParticipant = controller.participants().front().toMap();
        QVERIFY(localParticipant.value(QStringLiteral("isHost")).toBool());
        QVERIFY(localParticipant.value(QStringLiteral("isSelf")).toBool());
        QCOMPARE(localParticipant.value(QStringLiteral("displayName")).toString(), QStringLiteral("Marta"));
        QVERIFY(!controller.inviteEndpoints().isEmpty());
        QVERIFY(controller.inviteEndpoints().front().endsWith(QStringLiteral(":%1").arg(controller.hostingPort())));
        QCOMPARE(controller.inviteCode(), firstCode);
        const SessionCode code(firstCode.toStdString());

        SessionController otherWindow(data.path());
        otherWindow.openHostedCampaign(0);
        otherWindow.startHosting(0, QStringLiteral("Marta"));
        QTRY_VERIFY_WITH_TIMEOUT(!otherWindow.busy(), 5000);
        QCOMPARE(otherWindow.hostingPort(), 0);
        QVERIFY(!otherWindow.errorMessage().isEmpty());

        auto join = player.joinSession("127.0.0.1", static_cast<std::uint16_t>(controller.hostingPort()), code);
        QCOMPARE(join.wait_for(5s), std::future_status::ready);
        joined = join.get();
        QVERIFY(joined->userToken);
        QTRY_COMPARE_WITH_TIMEOUT(controller.participants().size(), 2, 5000);
        const auto playerRow = controller.participants().back().toMap();
        QCOMPARE(playerRow.value(QStringLiteral("userId")).toString(), QString::fromStdString(joined->userId));
        QVERIFY(!playerRow.value(QStringLiteral("isHost")).toBool());
        QVERIFY(!playerRow.value(QStringLiteral("isSelf")).toBool());

        auto info = player.sendCommand("session.info", nlohmann::json::object());
        QCOMPARE(info.wait_for(5s), std::future_status::ready);
        QCOMPARE(QString::fromStdString(info.get().at("name").get<std::string>()), name);

        controller.stopHosting();
        QTRY_VERIFY_WITH_TIMEOUT(player.getClientState().state == ClientConnectionState::Disconnected, 5000);
        QCOMPARE(controller.hostingPort(), 0);
        QVERIFY(controller.participants().isEmpty());
        QVERIFY(controller.inviteCode().isEmpty());
        QVERIFY(!controller.hostedCampaignOpen());
        QCOMPARE(controller.hostedCampaigns().size(), 1);
        QCOMPARE(controller.hostedCampaigns().front().toMap().value(QStringLiteral("name")).toString(), name);
    }

    SessionController reopened(data.path());
    reopened.openHostedCampaign(0);
    QCOMPARE(reopened.inviteCode(), firstCode);
    reopened.startHosting(0, QStringLiteral("Marta"));
    QTRY_VERIFY_WITH_TIMEOUT(!reopened.busy(), 5000);
    QVERIFY2(reopened.hostingPort() > 0, qPrintable(reopened.errorMessage()));
    QCOMPARE(reopened.inviteCode(), firstCode);
    const SessionCode code(firstCode.toStdString());
    auto resume = player.resumeSession("127.0.0.1", static_cast<std::uint16_t>(reopened.hostingPort()), code, *joined->userToken);
    QCOMPARE(resume.wait_for(5s), std::future_status::ready);
    const auto restored = resume.get();
    QCOMPARE(restored.userId, joined->userId);
    QVERIFY(!restored.userToken);
    QTRY_COMPARE_WITH_TIMEOUT(reopened.participants().size(), 2, 5000);
    player.disconnectFromHost();
    QTRY_COMPARE_WITH_TIMEOUT(reopened.participants().size(), 1, 5000);
}

QTEST_MAIN(SessionControllerTest)
#include "session_controller_test.moc"
