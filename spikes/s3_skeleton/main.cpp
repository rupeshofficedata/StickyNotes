// Spike S3: does a skeleton build and start against Qt + KF6 DBusAddons/StatusNotifierItem?
// Throwaway code. Uses QGuiApplication on purpose to see if KStatusNotifierItem needs Widgets.
#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QTimer>
#include <QTextStream>
#include <KDBusService>
#include <KStatusNotifierItem>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("rupeshofficedata");
    QCoreApplication::setOrganizationDomain("rupeshofficedata.github.io");
    QCoreApplication::setApplicationName("StickyNotes");
    QGuiApplication::setDesktopFileName("io.github.rupeshofficedata.StickyNotes");
    QTextStream out(stdout);

    KDBusService service(KDBusService::Unique);
    QObject::connect(&service, &KDBusService::activateRequested, &app, [&] { out << "activateRequested\n" << Qt::flush; });

    auto *bus = QDBusConnection::sessionBus().interface();
    out << "tray watcher registered: " << bus->isServiceRegistered("org.kde.StatusNotifierWatcher").value() << "\n";
    KStatusNotifierItem tray;
    tray.setIconByName("document-edit");
    tray.setStatus(KStatusNotifierItem::Active);
    out << "own names: ";
    for (const QString &n : bus->registeredServiceNames().value())
        if (n.contains("StickyNotes")) out << n << " ";
    out << "\n" << Qt::flush;
    QTimer::singleShot(1500, &app, &QCoreApplication::quit);
    return app.exec();
}
