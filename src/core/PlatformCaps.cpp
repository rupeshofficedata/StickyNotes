#include "PlatformCaps.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QGuiApplication>

namespace PlatformCaps {

bool trayPresent()
{
    const auto *bus = QDBusConnection::sessionBus().interface();
    // No session bus gives an invalid reply, which reads as false.
    return bus && bus->isServiceRegistered(QStringLiteral("org.kde.StatusNotifierWatcher")).value();
}

bool isWayland()
{
    return QGuiApplication::platformName() == QLatin1String("wayland");
}

} // namespace PlatformCaps
