#pragma once
#include <QObject>
#include <QPoint>
#include <QSet>
#include <QStringList>

class NoteStore;

// The only code that hides, shows or quits. QML windows report when they are shown or hidden and ask
// for actions; AppLifecycle answers with signals. Tray presence is decided once at startup and passed in.
class AppLifecycle : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool trayPresent READ trayPresent CONSTANT)

public:
    AppLifecycle(NoteStore *store, bool trayPresent, QObject *parent = nullptr);

    bool trayPresent() const { return m_tray; }

    Q_INVOKABLE void newNote();
    Q_INVOKABLE void openNote(const QString &id);
    Q_INVOKABLE void showPrimary();
    // The user closed the primary window: hide it, or quit when nothing else would stay visible.
    Q_INVOKABLE void primaryCloseRequested();
    // Flushes saves first. With force = false a failed save blocks the quit and emits quitBlocked.
    Q_INVOKABLE void quit(bool force = false);
    // Settings: close every note window (each saves first) before the notes folder changes.
    Q_INVOKABLE void closeAllNotes() { emit closeAllNotesRequested(); }

    // kind is "primary" or "note". A minimized window still counts as visible.
    Q_INVOKABLE void windowShown(const QString &kind, const QString &id);
    Q_INVOKABLE void windowHidden(const QString &kind, const QString &id);

    // (80, 80), then +32 px each, wrapping after 8 notes opened in this session. Wayland ignores it.
    Q_INVOKABLE QPoint nextCascadePosition();

signals:
    void showPrimaryRequested();
    void hidePrimaryRequested();
    void openNoteRequested(const QString &id);
    void closeAllNotesRequested();
    void quitBlocked(const QStringList &failedIds);
    void quitApproved();

private:
    int visibleCount() const { return static_cast<int>(m_notes.size()) + (m_primaryVisible ? 1 : 0); }
    void quitIfNothingVisible();

    NoteStore *m_store;
    bool m_tray;
    bool m_primaryVisible = false;
    QSet<QString> m_notes;
    int m_opened = 0;
    bool m_quitting = false;
};
