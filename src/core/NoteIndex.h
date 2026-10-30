#pragma once
#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QVector>

class NoteStore;

// List model of note summaries with a search filter. One instance lists the notes folder,
// a second one (bin = true) lists .bin. The Bin is never mixed into the main list or its search.
class NoteIndex : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, SnippetRole, ModifiedRole, ColorRole, IsErrorRole };

    NoteIndex(NoteStore *store, bool bin, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Rereads only notes whose modification time changed, then updates rows in place.
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void setFilter(const QString &text);

    // First non-empty line cut to 60 characters; "Untitled" for an empty note.
    static QString titleFrom(const QString &plainText);

private:
    struct Entry {
        QString id;          // note id, or bin name
        QString title;
        QString snippet;
        QString text;        // plain text, used by the search only
        qint64 mtimeMs = 0;
        QString color;
        bool isError = false;
        QString error;
    };

    Entry readEntry(const QString &id) const;
    QVector<Entry> visibleEntries() const;
    void applyRows(const QVector<Entry> &next);
    static bool sameRow(const Entry &a, const Entry &b);

    NoteStore *m_store;
    bool m_bin;
    QString m_filter;
    QHash<QString, Entry> m_cache;   // every note, filtered or not
    QVector<Entry> m_rows;           // rows currently shown
};
