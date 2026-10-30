#pragma once
#include <QImage>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

#include <memory>

class QSettings;

// Reads and writes note folders: <folder>/<id>/{note.html, meta.json, images/} and <folder>/.bin/<id>.
// All calls are synchronous and run on the GUI thread.
class NoteStore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder NOTIFY folderChanged)
    Q_PROPERTY(bool available READ available NOTIFY folderChanged)

public:
    static constexpr qint64 kMaxImageBytes = qint64(10) * 1024 * 1024;

    // defaultFolder is the only path the store will create when it is missing.
    // settings = nullptr uses the application QSettings. Tests pass their own.
    explicit NoteStore(const QString &defaultFolder, QSettings *settings = nullptr, QObject *parent = nullptr);
    ~NoteStore() override;

    QString folder() const { return m_folder; }
    bool available() const { return m_available; }

    Q_INVOKABLE bool setFolder(const QString &path);
    // Startup: uses the saved folder, or the default one.
    void restoreSavedFolder();
    // Settings page: switches to the folder and remembers it. Does not copy, move or delete notes.
    Q_INVOKABLE bool changeFolder(const QString &path);

    Q_INVOKABLE QString createNote();
    // Keys: ok, html, colorToken, mtimeMs, sizeBytes, error.
    Q_INVOKABLE QVariantMap load(const QString &id) const;
    QVariantMap loadAt(const QString &dir) const;
    Q_INVOKABLE bool save(const QString &id, const QString &html, const QString &colorToken);
    // Asks every open note window to save now (flushRequested is a direct, synchronous signal),
    // then returns the ids whose latest save failed.
    QStringList flushAll();

    // Notes that are open in a window. checkOpenNotes() compares each note's files with the stamp recorded
    // at load or after the last own save, and emits noteChangedOutside / noteRemovedOutside once per change.
    Q_INVOKABLE void trackOpen(const QString &id);
    Q_INVOKABLE void untrack(const QString &id);
    Q_INVOKABLE void checkOpenNotes();

    // Window size by note id, and position only when savePosition is true (X11).
    Q_INVOKABLE void saveWindowState(const QString &id, int w, int h, int x, int y, bool savePosition);
    Q_INVOKABLE QVariantMap windowState(const QString &id) const;

    Q_INVOKABLE bool moveToBin(const QString &id);
    Q_INVOKABLE bool restoreFromBin(const QString &binName);
    Q_INVOKABLE bool deleteForever(const QString &binName);
    Q_INVOKABLE void emptyBin();

    // Returns "images/<uuid>.png", or an empty string when the image is rejected or not an image.
    QString addImage(const QString &id, const QImage &image);
    Q_INVOKABLE QString addImageFile(const QString &id, const QString &path);

    Q_INVOKABLE QString noteDir(const QString &id) const;
    QString binDir(const QString &binName) const;
    Q_INVOKABLE QUrl noteUrl(const QString &id) const;
    QStringList noteIds() const;
    QStringList binNames() const;

signals:
    void folderChanged();
    void saveFailed(const QString &id, const QString &reason);
    void storeUnavailable(const QString &reason);
    void restoreRefused(const QString &binName, const QString &reason);
    void imageRejected(const QString &id, const QString &reason);
    void flushRequested();
    void noteChangedOutside(const QString &id);
    void noteRemovedOutside(const QString &id);

private:
    struct FileStamp {
        qint64 mtimeMs = -1;
        qint64 size = -1;
        bool operator==(const FileStamp &o) const { return mtimeMs == o.mtimeMs && size == o.size; }
    };
    struct Stamp {
        FileStamp note, meta;
        bool operator==(const Stamp &o) const { return note == o.note && meta == o.meta; }
    };
    Stamp stampOf(const QString &id) const;
    bool saveImpl(const QString &id, const QString &html, const QString &colorToken);
    void markUnavailable(const QString &path, const QString &reason);
    QStringList idsIn(const QString &dir) const;

    QString m_default;
    QString m_folder;
    bool m_available = false;
    QSet<QString> m_failed;
    QHash<QString, Stamp> m_open;
    std::unique_ptr<QSettings> m_ownSettings;
    QSettings *m_settings;
};
