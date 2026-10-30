#include "NoteIndex.h"
#include "NoteStore.h"

#include <QFileInfo>
#include <QSet>
#include <QTextDocument>
#include <algorithm>

NoteIndex::NoteIndex(NoteStore *store, bool bin, QObject *parent)
    : QAbstractListModel(parent), m_store(store), m_bin(bin)
{
}

int NoteIndex::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant NoteIndex::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Entry &e = m_rows.at(index.row());
    switch (role) {
    case IdRole: return e.id;
    case TitleRole: return e.isError ? tr("Unreadable note") : e.title;
    case SnippetRole: return e.isError ? e.error : e.snippet;
    case ModifiedRole: return QDateTime::fromMSecsSinceEpoch(e.mtimeMs);
    case ColorRole: return e.color;
    case IsErrorRole: return e.isError;
    default: return {};
    }
}

QHash<int, QByteArray> NoteIndex::roleNames() const
{
    return {{IdRole, "noteId"}, {TitleRole, "title"}, {SnippetRole, "snippet"},
            {ModifiedRole, "modified"}, {ColorRole, "colorToken"}, {IsErrorRole, "isError"}};
}

QString NoteIndex::titleFrom(const QString &plainText)
{
    const auto lines = plainText.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString t = line.trimmed();
        if (!t.isEmpty())
            return t.left(60);
    }
    return tr("Untitled");
}

NoteIndex::Entry NoteIndex::readEntry(const QString &id) const
{
    Entry e;
    e.id = id;
    const QVariantMap m = m_store->loadAt(m_bin ? m_store->binDir(id) : m_store->noteDir(id));
    e.color = m.value(QStringLiteral("colorToken")).toString();
    if (!m.value(QStringLiteral("ok")).toBool()) {
        e.isError = true;
        e.error = m.value(QStringLiteral("error")).toString();
        return e;
    }
    e.mtimeMs = m.value(QStringLiteral("mtimeMs")).toLongLong();
    QTextDocument doc;
    doc.setHtml(m.value(QStringLiteral("html")).toString());
    e.text = doc.toPlainText();
    e.title = titleFrom(e.text);
    e.snippet = e.text.simplified().left(100);
    return e;
}

bool NoteIndex::sameRow(const Entry &a, const Entry &b)
{
    return a.id == b.id && a.mtimeMs == b.mtimeMs && a.isError == b.isError && a.color == b.color;
}

QVector<NoteIndex::Entry> NoteIndex::visibleEntries() const
{
    QVector<Entry> out;
    for (const Entry &e : m_cache) {
        if (m_filter.isEmpty() || e.title.contains(m_filter, Qt::CaseInsensitive)
            || e.text.contains(m_filter, Qt::CaseInsensitive))
            out.append(e);
    }
    // Newest first. Unreadable notes have mtime 0 and sort last. Ties break on id so the order is stable.
    std::sort(out.begin(), out.end(), [](const Entry &a, const Entry &b) {
        return a.mtimeMs != b.mtimeMs ? a.mtimeMs > b.mtimeMs : a.id < b.id;
    });
    return out;
}

void NoteIndex::applyRows(const QVector<Entry> &next)
{
    // Update the model in place (remove, move, insert, dataChanged) so the view keeps its scroll position.
    QSet<QString> wanted;
    for (const Entry &e : next)
        wanted.insert(e.id);

    for (int i = static_cast<int>(m_rows.size()) - 1; i >= 0; --i) {
        if (!wanted.contains(m_rows.at(i).id)) {
            beginRemoveRows({}, i, i);
            m_rows.removeAt(i);
            endRemoveRows();
        }
    }
    for (int i = 0; i < next.size(); ++i) {
        if (i < m_rows.size() && m_rows.at(i).id == next.at(i).id) {
            if (!sameRow(m_rows.at(i), next.at(i))) {
                m_rows[i] = next.at(i);
                emit dataChanged(index(i), index(i));
            }
            continue;
        }
        int from = -1;
        for (int j = i + 1; j < m_rows.size(); ++j) {
            if (m_rows.at(j).id == next.at(i).id) {
                from = j;
                break;
            }
        }
        if (from >= 0) {
            beginMoveRows({}, from, from, {}, i);
            m_rows.move(from, i);
            endMoveRows();
            if (!sameRow(m_rows.at(i), next.at(i))) {
                m_rows[i] = next.at(i);
                emit dataChanged(index(i), index(i));
            }
        } else {
            beginInsertRows({}, i, i);
            m_rows.insert(i, next.at(i));
            endInsertRows();
        }
    }
}

void NoteIndex::rescan()
{
    const QStringList ids = m_bin ? m_store->binNames() : m_store->noteIds();
    QHash<QString, Entry> fresh;
    for (const QString &id : ids) {
        const QString dir = m_bin ? m_store->binDir(id) : m_store->noteDir(id);
        const QFileInfo fi(dir + QStringLiteral("/note.html"));
        const auto it = m_cache.constFind(id);
        // Reread only when the modification time changed. A note that failed to parse is retried every rescan.
        if (it != m_cache.constEnd() && !it->isError && fi.exists()
            && it->mtimeMs == fi.lastModified().toMSecsSinceEpoch()) {
            fresh.insert(id, *it);
        } else {
            fresh.insert(id, readEntry(id));
        }
    }
    m_cache = std::move(fresh);
    applyRows(visibleEntries());
}

void NoteIndex::setFilter(const QString &text)
{
    if (text == m_filter)
        return;
    m_filter = text;
    applyRows(visibleEntries());
}
