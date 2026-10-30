#include "AppLifecycle.h"
#include "Logging.h"
#include "NoteStore.h"

Q_LOGGING_CATEGORY(lcTray, "stickynotes.tray")
Q_LOGGING_CATEGORY(lcActivation, "stickynotes.activation")

AppLifecycle::AppLifecycle(NoteStore *store, bool trayPresent, QObject *parent)
    : QObject(parent), m_store(store), m_tray(trayPresent)
{
}

void AppLifecycle::newNote()
{
    const QString id = m_store->createNote();
    if (!id.isEmpty())
        emit openNoteRequested(id);
}

void AppLifecycle::openNote(const QString &id)
{
    emit openNoteRequested(id);
}

void AppLifecycle::showPrimary()
{
    qCInfo(lcActivation) << "showPrimary";
    emit showPrimaryRequested();
}

void AppLifecycle::primaryCloseRequested()
{
    // With a tray the app keeps running. Without one, hiding the last window would leave no way back.
    if (m_tray || !m_notes.isEmpty())
        emit hidePrimaryRequested();
    else
        quit();
}

void AppLifecycle::windowShown(const QString &kind, const QString &id)
{
    if (kind == QLatin1String("primary"))
        m_primaryVisible = true;
    else
        m_notes.insert(id);
}

void AppLifecycle::windowHidden(const QString &kind, const QString &id)
{
    if (kind == QLatin1String("primary"))
        m_primaryVisible = false;
    else
        m_notes.remove(id);
    quitIfNothingVisible();
}

void AppLifecycle::quitIfNothingVisible()
{
    if (!m_tray && visibleCount() == 0)
        quit();
}

void AppLifecycle::quit(bool force)
{
    if (m_quitting)
        return;
    const QStringList failed = m_store->flushAll();
    if (!force && !failed.isEmpty()) {
        emit quitBlocked(failed);
        return;
    }
    m_quitting = true;
    emit quitApproved();
}

QPoint AppLifecycle::nextCascadePosition()
{
    const int step = m_opened++ % 8;
    return QPoint(80 + 32 * step, 80 + 32 * step);
}
