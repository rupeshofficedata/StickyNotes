#pragma once
#include <QLoggingCategory>

// Turn on with QT_LOGGING_RULES="stickynotes.*=true". Defined in NoteStore.cpp and AppLifecycle.cpp.
Q_DECLARE_LOGGING_CATEGORY(lcStorage)
Q_DECLARE_LOGGING_CATEGORY(lcTray)
Q_DECLARE_LOGGING_CATEGORY(lcActivation)
