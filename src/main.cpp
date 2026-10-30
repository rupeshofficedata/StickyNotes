#include "AppLifecycle.h"
#include "Logging.h"
#include "NoteIndex.h"
#include "NoteStore.h"
#include "PlatformCaps.h"
#include "RichTextBridge.h"

#include <KDBusService>
#include <KStatusNotifierItem>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QtQml>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int g_signalFd[2];

void onUnixSignal(int)
{
    const char c = 1;
    (void)!::write(g_signalFd[0], &c, 1);   // only async-signal-safe work here
}

// SIGTERM and SIGINT cannot call Qt directly. The handler writes a byte to a socket pair and a
// QSocketNotifier turns it into a normal Qt callback.
void installSignalBridge(QObject *parent, std::function<void()> onTerminate)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, g_signalFd) != 0)
        return;
    auto *notifier = new QSocketNotifier(g_signalFd[1], QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated, parent, [onTerminate] {
        char c;
        (void)!::read(g_signalFd[1], &c, 1);
        onTerminate();
    });
    struct sigaction sa {};
    sa.sa_handler = onUnixSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);
}

} // namespace

int main(int argc, char *argv[])
{
    // KStatusNotifierItem has a context menu, so the app needs QApplication and Qt Widgets.
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("rupeshofficedata"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("rupeshofficedata.github.io"));
    QCoreApplication::setApplicationName(QStringLiteral("StickyNotes"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.rupeshofficedata.StickyNotes"));
    // AppLifecycle decides when the app quits.
    app.setQuitOnLastWindowClosed(false);

    // Unique mode: a second launch asks this instance to show its primary window and exits with status 0.
    KDBusService service(KDBusService::Unique);

    const QString defaultFolder = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                                  + QStringLiteral("/io.github.rupeshofficedata.StickyNotes/notes");
    NoteStore store(defaultFolder);
    store.restoreSavedFolder();
    NoteIndex index(&store, false);
    index.rescan();
    NoteIndex binIndex(&store, true);
    binIndex.rescan();

    // Tray presence is checked once. If the tray disappears later the behavior does not change.
    const bool tray = PlatformCaps::trayPresent();
    qCInfo(lcTray) << "tray present:" << tray;
    AppLifecycle lifecycle(&store, tray);
    QObject::connect(&lifecycle, &AppLifecycle::quitApproved, &app, &QCoreApplication::quit);
    QObject::connect(&service, &KDBusService::activateRequested, &lifecycle, [&lifecycle] { lifecycle.showPrimary(); });
    // Last flush on every exit path. It never opens a dialog.
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &store, [&store] { store.flushAll(); });
    // Rescan when the app becomes active again: a sync client may have changed files meanwhile.
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &store,
                     [&store, &index, &binIndex](Qt::ApplicationState state) {
                         if (state != Qt::ApplicationActive)
                             return;
                         store.checkOpenNotes();
                         index.rescan();
                         binIndex.rescan();
                     });
    installSignalBridge(&app, [&lifecycle] { lifecycle.quit(true); });
    // Session logout: the same forced flush.
    QObject::connect(&app, &QGuiApplication::commitDataRequest, &lifecycle, [&lifecycle] { lifecycle.quit(true); });

    KStatusNotifierItem *trayItem = nullptr;
    if (tray) {
        trayItem = new KStatusNotifierItem(&app);
        trayItem->setCategory(KStatusNotifierItem::ApplicationStatus);
        trayItem->setIconByName(QStringLiteral("document-edit"));
        trayItem->setTitle(QStringLiteral("StickyNotes"));
        trayItem->setStandardActionsEnabled(false);   // the default Quit would bypass AppLifecycle
        auto *menu = new QMenu;
        menu->addAction(QObject::tr("Show StickyNotes"), &lifecycle, [&lifecycle] { lifecycle.showPrimary(); });
        menu->addAction(QObject::tr("New note"), &lifecycle, [&lifecycle] { lifecycle.newNote(); });
        menu->addSeparator();
        menu->addAction(QObject::tr("Quit"), &lifecycle, [&lifecycle] { lifecycle.quit(); });
        trayItem->setContextMenu(menu);
        QObject::connect(trayItem, &KStatusNotifierItem::activateRequested, &lifecycle,
                         [&lifecycle] { lifecycle.showPrimary(); });
    }

    qmlRegisterSingletonInstance("StickyNotes", 1, 0, "NoteStore", &store);
    qmlRegisterSingletonInstance("StickyNotes", 1, 0, "NoteIndex", &index);
    qmlRegisterSingletonInstance("StickyNotes", 1, 0, "BinIndex", &binIndex);
    qmlRegisterSingletonInstance("StickyNotes", 1, 0, "Lifecycle", &lifecycle);
    qmlRegisterType<RichTextBridge>("StickyNotes", 1, 0, "RichTextBridge");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("StickyNotes", "Main");
    return app.exec();
}
