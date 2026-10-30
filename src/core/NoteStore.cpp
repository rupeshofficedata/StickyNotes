#include "NoteStore.h"
#include "Logging.h"

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QSaveFile>
#include <QUuid>
#include <algorithm>

Q_LOGGING_CATEGORY(lcStorage, "stickynotes.storage")

namespace {

const QStringList &colorTokens()
{
    static const QStringList tokens{QStringLiteral("default"), QStringLiteral("blue"), QStringLiteral("green"),
                                    QStringLiteral("pink")};
    return tokens;
}

// A note id is a lowercase UUID. A bin name may carry a ".<unix-ms>" suffix.
bool isNoteId(const QString &s)
{
    static const QRegularExpression re(
        QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$"));
    return re.match(s).hasMatch();
}

bool isBinName(const QString &s)
{
    static const QRegularExpression re(
        QStringLiteral("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}(\\.\\d+)?$"));
    return re.match(s).hasMatch();
}

// The notes folder may be shared or synced, so another party could plant symlinks in it. The app never
// reads, writes, lists or deletes through one.
bool isSymlink(const QString &path)
{
    return QFileInfo(path).isSymLink();
}

bool writeAtomic(const QString &path, const QByteArray &data, QString *error)
{
    if (isSymlink(path)) {
        *error = QStringLiteral("Refusing to write through a symbolic link: ") + path;
        return false;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        *error = f.errorString();
        return false;
    }
    f.write(data);
    if (!f.commit()) {
        *error = f.errorString();
        return false;
    }
    return true;
}

} // namespace

NoteStore::NoteStore(const QString &defaultFolder, QSettings *settings, QObject *parent)
    : QObject(parent), m_default(QDir::cleanPath(defaultFolder)), m_settings(settings)
{
    if (!m_settings) {
        m_ownSettings = std::make_unique<QSettings>();
        m_settings = m_ownSettings.get();
    }
}

NoteStore::~NoteStore() = default;

void NoteStore::restoreSavedFolder()
{
    setFolder(m_settings->value(QStringLiteral("notesFolder"), m_default).toString());
}

bool NoteStore::changeFolder(const QString &path)
{
    if (!setFolder(path))
        return false;
    m_settings->setValue(QStringLiteral("notesFolder"), folder());
    m_open.clear();
    m_failed.clear();
    return true;
}

NoteStore::Stamp NoteStore::stampOf(const QString &id) const
{
    auto stamp = [](const QString &path) {
        const QFileInfo fi(path);
        return fi.exists() ? FileStamp{fi.lastModified().toMSecsSinceEpoch(), fi.size()} : FileStamp{};
    };
    const QString dir = noteDir(id);
    return {stamp(dir + QStringLiteral("/note.html")), stamp(dir + QStringLiteral("/meta.json"))};
}

void NoteStore::trackOpen(const QString &id)
{
    if (!noteDir(id).isEmpty())
        m_open.insert(id, stampOf(id));
}

void NoteStore::untrack(const QString &id)
{
    m_open.remove(id);
}

void NoteStore::checkOpenNotes()
{
    const QStringList ids = m_open.keys();
    for (const QString &id : ids) {
        const Stamp now = stampOf(id);
        if (now == m_open.value(id))
            continue;
        // Record the new state first, so each outside change is reported once.
        m_open[id] = now;
        if (now.note.size < 0)
            emit noteRemovedOutside(id);
        else
            emit noteChangedOutside(id);
    }
}

void NoteStore::saveWindowState(const QString &id, int w, int h, int x, int y, bool savePosition)
{
    if (noteDir(id).isEmpty())
        return;
    m_settings->beginGroup(QStringLiteral("window/") + id);
    m_settings->setValue(QStringLiteral("width"), w);
    m_settings->setValue(QStringLiteral("height"), h);
    if (savePosition) {
        m_settings->setValue(QStringLiteral("x"), x);
        m_settings->setValue(QStringLiteral("y"), y);
    }
    m_settings->endGroup();
}

QVariantMap NoteStore::windowState(const QString &id) const
{
    QVariantMap out;
    if (noteDir(id).isEmpty())
        return out;
    m_settings->beginGroup(QStringLiteral("window/") + id);
    for (const char *key : {"width", "height", "x", "y"}) {
        if (m_settings->contains(QLatin1String(key)))
            out.insert(QLatin1String(key), m_settings->value(QLatin1String(key)).toInt());
    }
    m_settings->endGroup();
    return out;
}

void NoteStore::markUnavailable(const QString &path, const QString &reason)
{
    m_folder = path;
    m_available = false;
    qCWarning(lcStorage) << "notes folder unavailable:" << path << reason;
    emit storeUnavailable(reason);
    emit folderChanged();
}

bool NoteStore::setFolder(const QString &path)
{
    const QString clean = QDir::cleanPath(path);
    if (!QFileInfo::exists(clean)) {
        // The default folder is the only one the app creates.
        if (clean != m_default || !QDir().mkpath(clean)) {
            markUnavailable(clean, tr("The folder does not exist: %1").arg(clean));
            return false;
        }
    }
    const QFileInfo fi(clean);
    if (!fi.isDir() || !fi.isWritable()) {
        markUnavailable(clean, tr("The folder is not a writable directory: %1").arg(clean));
        return false;
    }
    m_folder = clean;
    m_available = true;
    emit folderChanged();
    return true;
}

QString NoteStore::noteDir(const QString &id) const
{
    return isNoteId(id) ? m_folder + QLatin1Char('/') + id : QString();
}

QString NoteStore::binRoot() const
{
    if (!m_available)
        return {};
    QString bin = m_folder + QStringLiteral("/.bin");
    const QFileInfo fi(bin);
    if (!fi.exists() && !fi.isSymLink())
        return bin;   // not created yet
    // Compare with where .bin must be, not with wherever a link points.
    if (fi.isSymLink() || !fi.isDir()
        || QDir(bin).canonicalPath() != QDir(m_folder).canonicalPath() + QStringLiteral("/.bin"))
        return {};
    return bin;
}

QString NoteStore::binDir(const QString &binName) const
{
    const QString root = binRoot();
    return (root.isEmpty() || !isBinName(binName)) ? QString() : root + QLatin1Char('/') + binName;
}

QUrl NoteStore::noteUrl(const QString &id) const
{
    return QUrl::fromLocalFile(noteDir(id) + QLatin1Char('/'));
}

QString NoteStore::createNote()
{
    if (!m_available)
        return {};
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (!QDir().mkpath(noteDir(id) + QStringLiteral("/images")))
        return {};
    return save(id, QString(), QStringLiteral("default")) ? id : QString();
}

bool NoteStore::save(const QString &id, const QString &html, const QString &colorToken)
{
    const bool ok = saveImpl(id, html, colorToken);
    if (ok) {
        m_failed.remove(id);
        if (m_open.contains(id))
            m_open[id] = stampOf(id);   // our own write is not an outside change
    }
    else
        m_failed.insert(id);
    return ok;
}

QStringList NoteStore::flushAll()
{
    emit flushRequested();
    QStringList ids = m_failed.values();
    ids.sort();
    return ids;
}

bool NoteStore::saveImpl(const QString &id, const QString &html, const QString &colorToken)
{
    const QString dir = noteDir(id);
    if (!m_available || dir.isEmpty()) {
        emit saveFailed(id, tr("The notes folder is not available"));
        return false;
    }
    QString error;
    // meta.json first, note.html second. A crash in between keeps the old text with the new color.
    // Saving a note whose folder was removed outside recreates the folder.
    if (!QDir().mkpath(dir)) {
        emit saveFailed(id, tr("Cannot create the note folder"));
        return false;
    }
    const QString token = colorTokens().contains(colorToken) ? colorToken : QStringLiteral("default");
    QJsonObject meta;
    meta.insert(QStringLiteral("color"), token);
    if (!writeAtomic(dir + QStringLiteral("/meta.json"), QJsonDocument(meta).toJson(QJsonDocument::Compact),
                     &error)
        || !writeAtomic(dir + QStringLiteral("/note.html"), html.toUtf8(), &error)) {
        qCWarning(lcStorage) << "save failed" << id << error;
        emit saveFailed(id, error);
        return false;
    }
    return true;
}

QVariantMap NoteStore::load(const QString &id) const
{
    const QString dir = noteDir(id);
    if (dir.isEmpty())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Invalid note id")}};
    return loadAt(dir);
}

QVariantMap NoteStore::loadAt(const QString &dir) const
{
    if (dir.isEmpty())
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Invalid note folder")}};
    QVariantMap out{{QStringLiteral("ok"), false},
                    {QStringLiteral("html"), QString()},
                    {QStringLiteral("colorToken"), QStringLiteral("default")},
                    {QStringLiteral("mtimeMs"), 0},
                    {QStringLiteral("sizeBytes"), 0},
                    {QStringLiteral("error"), QString()}};

    // A missing, unreadable or unknown meta.json is not fatal: the note loads with the default color.
    QFile meta(dir + QStringLiteral("/meta.json"));
    if (!isSymlink(meta.fileName()) && meta.open(QIODevice::ReadOnly)) {
        const QString token = QJsonDocument::fromJson(meta.readAll()).object().value(QStringLiteral("color")).toString();
        if (colorTokens().contains(token))
            out[QStringLiteral("colorToken")] = token;
    }

    QFile note(dir + QStringLiteral("/note.html"));
    if (isSymlink(dir) || isSymlink(note.fileName())) {
        out[QStringLiteral("error")] = tr("Symbolic links are not followed");
        return out;
    }
    if (!note.open(QIODevice::ReadOnly)) {
        out[QStringLiteral("error")] = note.errorString();
        return out;
    }
    const QFileInfo fi(note);
    out[QStringLiteral("html")] = QString::fromUtf8(note.readAll());
    out[QStringLiteral("mtimeMs")] = fi.lastModified().toMSecsSinceEpoch();
    out[QStringLiteral("sizeBytes")] = fi.size();
    out[QStringLiteral("ok")] = true;
    return out;
}

bool NoteStore::moveToBin(const QString &id)
{
    const QString dir = noteDir(id);
    if (!m_available || dir.isEmpty() || isSymlink(dir) || !QFileInfo(dir).isDir())
        return false;
    QDir root(m_folder);
    if (binRoot().isEmpty() || !root.mkpath(QStringLiteral(".bin")) || binRoot().isEmpty())
        return false;   // .bin is a symlink or resolves outside the notes folder
    const QString target = QStringLiteral(".bin/") + id;
    if (root.exists(target)) {
        // Never overwrite an older bin entry: keep it under a timestamp suffix.
        root.rename(target, target + QLatin1Char('.') + QString::number(QDateTime::currentMSecsSinceEpoch()));
    }
    return root.rename(id, target);
}

bool NoteStore::restoreFromBin(const QString &binName)
{
    const QString src = binDir(binName);
    if (!m_available || src.isEmpty() || isSymlink(src) || !QFileInfo(src).isDir())
        return false;
    const QString id = binName.left(36);
    if (QFileInfo::exists(noteDir(id))) {
        emit restoreRefused(binName, tr("A note with this ID already exists."));
        return false;
    }
    return QDir(m_folder).rename(QStringLiteral(".bin/") + binName, id);
}

bool NoteStore::deleteForever(const QString &binName)
{
    const QString dir = binDir(binName);
    if (!m_available || dir.isEmpty() || isSymlink(dir) || !QFileInfo(dir).isDir())
        return false;
    // The entry must really live in <notes folder>/.bin, whatever the paths say.
    const QString expected = QDir(m_folder).canonicalPath() + QStringLiteral("/.bin/");
    if (!QFileInfo(dir).canonicalFilePath().startsWith(expected))
        return false;
    if (!QDir(dir).removeRecursively())
        return false;
    m_settings->remove(QStringLiteral("window/") + binName.left(36));
    return true;
}

void NoteStore::emptyBin()
{
    const auto names = binNames();
    for (const QString &name : names)
        deleteForever(name);
}

QString NoteStore::addImage(const QString &id, const QImage &image)
{
    const QString dir = noteDir(id);
    if (!m_available || dir.isEmpty() || image.isNull())
        return {};
    if (isSymlink(dir) || isSymlink(dir + QStringLiteral("/images"))) {
        emit imageRejected(id, tr("Symbolic links are not followed"));
        return {};
    }
    QByteArray png;
    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    if (png.size() > kMaxImageBytes) {
        emit imageRejected(id, tr("The image is larger than 10 MB"));
        return {};
    }
    QString rel = QStringLiteral("images/") + QUuid::createUuid().toString(QUuid::WithoutBraces)
                        + QStringLiteral(".png");
    QString error;
    if (!QDir().mkpath(dir + QStringLiteral("/images")) || !writeAtomic(dir + QLatin1Char('/') + rel, png, &error)) {
        emit imageRejected(id, error);
        return {};
    }
    return rel;
}

QString NoteStore::addImageFile(const QString &id, const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.isFile())
        return {};
    if (fi.size() > kMaxImageBytes) {
        emit imageRejected(id, tr("The image is larger than 10 MB"));
        return {};
    }
    // Non-image files fail to decode and are ignored. Non-PNG images are re-encoded to PNG.
    return addImage(id, QImage(path));
}

QStringList NoteStore::idsIn(const QString &dir) const
{
    QStringList ids = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    ids.erase(std::remove_if(ids.begin(), ids.end(), [](const QString &s) { return !isBinName(s); }), ids.end());
    return ids;
}

QStringList NoteStore::noteIds() const
{
    QStringList ids = m_available ? idsIn(m_folder) : QStringList();
    ids.erase(std::remove_if(ids.begin(), ids.end(), [](const QString &s) { return !isNoteId(s); }), ids.end());
    return ids;
}

QStringList NoteStore::binNames() const
{
    const QString root = binRoot();
    return root.isEmpty() ? QStringList() : idsIn(root);
}
