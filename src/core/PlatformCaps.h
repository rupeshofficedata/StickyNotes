#pragma once

// Facts about the session. Other code asks here and never checks the desktop by name.
namespace PlatformCaps {

// True when org.kde.StatusNotifierWatcher is registered on the session bus.
bool trayPresent();
bool isWayland();

} // namespace PlatformCaps
