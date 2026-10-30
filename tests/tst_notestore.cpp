#include "NoteStore.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QSettings>
#include <QTest>

class TstNoteStore : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(m_tmp.isValid());
        m_root = m_tmp.path() + QStringLiteral("/notes");
        QVERIFY(QDir().mkpath(m_root));
        m_store = new NoteStore(m_root, new QSettings(m_tmp.path() + QStringLiteral("/s.ini"), QSettings::IniFormat, this), this);
        QVERIFY(m_store->setFolder(m_root));
    }
    void cleanup()
    {
        delete m_store;
        m_store = nullptr;
        QDir(m_tmp.path()).removeRecursively();
    }

    void saveThenLoadReturnsSameHtml()
    {
        const QString id = m_store->createNote();
        QVERIFY(!id.isEmpty());
        QVERIFY(m_store->save(id, QStringLiteral("<p>hello</p>"), QStringLiteral("blue")));
        const QVariantMap d = m_store->load(id);
        QVERIFY(d.value("ok").toBool());
        QCOMPARE(d.value("html").toString(), QStringLiteral("<p>hello</p>"));
        QCOMPARE(d.value("colorToken").toString(), QStringLiteral("blue"));
    }

    void secondSaveLeavesBytesUnchanged()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, QStringLiteral("<p>x</p>"), QStringLiteral("pink")));
        const QByteArray first = readAll(m_store->noteDir(id) + "/note.html") + readAll(m_store->noteDir(id) + "/meta.json");
        QVERIFY(m_store->save(id, QStringLiteral("<p>x</p>"), QStringLiteral("pink")));
        const QByteArray second = readAll(m_store->noteDir(id) + "/note.html") + readAll(m_store->noteDir(id) + "/meta.json");
        QCOMPARE(second, first);
    }

    void saveIntoReadOnlyFolderEmitsSaveFailedAndKeepsOldContent()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, QStringLiteral("<p>old</p>"), QStringLiteral("default")));
        const QString dir = m_store->noteDir(id);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::ExeOwner);
        if (QFileInfo(dir).isWritable())
            QSKIP("running as a user that ignores directory permissions");
        QSignalSpy spy(m_store, &NoteStore::saveFailed);
        QVERIFY(!m_store->save(id, QStringLiteral("<p>new</p>"), QStringLiteral("default")));
        QCOMPARE(spy.count(), 1);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        QCOMPARE(m_store->load(id).value("html").toString(), QStringLiteral("<p>old</p>"));
    }

    void unreadableNoteReportsNotOk()
    {
        const QString id = m_store->createNote();
        QVERIFY(QFile::remove(m_store->noteDir(id) + "/note.html"));
        QVERIFY(!m_store->load(id).value("ok").toBool());
    }

    void missingUserFolderIsUnavailableAndNotCreated()
    {
        QSignalSpy spy(m_store, &NoteStore::storeUnavailable);
        const QString other = m_tmp.path() + QStringLiteral("/other");
        QVERIFY(!m_store->setFolder(other));
        QCOMPARE(spy.count(), 1);
        QVERIFY(!m_store->available());
        QVERIFY(!QFileInfo::exists(other));
        QVERIFY(m_store->createNote().isEmpty());
    }

    void missingDefaultFolderIsCreated()
    {
        const QString def = m_tmp.path() + QStringLiteral("/fresh/default");
        NoteStore s(def);
        QVERIFY(s.setFolder(def));
        QVERIFY(QFileInfo(def).isDir());
    }

    void invalidIdsAreRejected()
    {
        QVERIFY(m_store->noteDir(QStringLiteral("../etc")).isEmpty());
        QVERIFY(!m_store->save(QStringLiteral("not-a-uuid"), QString(), QStringLiteral("default")));
        QVERIFY(!m_store->moveToBin(QStringLiteral("../x")));
    }

    void invalidMetaFallsBackToDefault()
    {
        const QString id = m_store->createNote();
        QVERIFY(writeFile(m_store->noteDir(id) + "/meta.json", "{\"color\":\"purple\"}"));
        QCOMPARE(m_store->load(id).value("colorToken").toString(), QStringLiteral("default"));
        QVERIFY(writeFile(m_store->noteDir(id) + "/meta.json", "not json"));
        QCOMPARE(m_store->load(id).value("colorToken").toString(), QStringLiteral("default"));
        QFile::remove(m_store->noteDir(id) + "/meta.json");
        QVERIFY(m_store->load(id).value("ok").toBool());
    }

    void binMoveAndRestore()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->moveToBin(id));
        QVERIFY(!m_store->noteIds().contains(id));
        QCOMPARE(m_store->binNames(), QStringList{id});
        QVERIFY(m_store->restoreFromBin(id));
        QVERIFY(m_store->noteIds().contains(id));
        QVERIFY(m_store->binNames().isEmpty());
    }

    void restoreIsRefusedWhenIdExists()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->moveToBin(id));
        QVERIFY(QDir().mkpath(m_store->noteDir(id)));   // the id came back, for example through a sync client
        QSignalSpy spy(m_store, &NoteStore::restoreRefused);
        QVERIFY(!m_store->restoreFromBin(id));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(m_store->binNames(), QStringList{id});
    }

    void existingBinEntryIsRenamedWithSuffix()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->moveToBin(id));
        QVERIFY(QDir().mkpath(m_store->noteDir(id)));
        QVERIFY(m_store->save(id, QStringLiteral("<p>second</p>"), QStringLiteral("default")));
        QVERIFY(m_store->moveToBin(id));
        const QStringList names = m_store->binNames();
        QCOMPARE(names.size(), 2);
        QVERIFY(names.contains(id));
        QVERIFY(std::any_of(names.begin(), names.end(), [&](const QString &n) { return n.startsWith(id + '.'); }));
    }

    void emptyBinRemovesEverything()
    {
        m_store->moveToBin(m_store->createNote());
        m_store->moveToBin(m_store->createNote());
        QCOMPARE(m_store->binNames().size(), 2);
        m_store->emptyBin();
        QVERIFY(m_store->binNames().isEmpty());
    }

    void imageOver10MbIsRejected()
    {
        const QString id = m_store->createNote();
        const QString big = m_tmp.path() + "/big.png";
        QFile f(big);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.resize(NoteStore::kMaxImageBytes + 1);
        f.close();
        QSignalSpy spy(m_store, &NoteStore::imageRejected);
        QVERIFY(m_store->addImageFile(id, big).isEmpty());
        QCOMPARE(spy.count(), 1);
    }

    void nonImageDropIsIgnored()
    {
        const QString id = m_store->createNote();
        const QString txt = m_tmp.path() + "/a.txt";
        QVERIFY(writeFile(txt, "hello"));
        QVERIFY(m_store->addImageFile(id, txt).isEmpty());
    }

    void imageIsStoredAsPngUnderImages()
    {
        const QString id = m_store->createNote();
        QImage img(8, 8, QImage::Format_ARGB32);
        img.fill(Qt::red);
        const QString rel = m_store->addImage(id, img);
        QVERIFY(rel.startsWith(QStringLiteral("images/")) && rel.endsWith(QStringLiteral(".png")));
        QVERIFY(QFileInfo::exists(m_store->noteDir(id) + '/' + rel));
    }

    void savingAfterOutsideRemovalRecreatesTheNote()
    {
        const QString id = m_store->createNote();
        QVERIFY(QDir(m_store->noteDir(id)).removeRecursively());
        QVERIFY(m_store->save(id, QStringLiteral("<p>back</p>"), QStringLiteral("green")));
        QCOMPARE(m_store->load(id).value("html").toString(), QStringLiteral("<p>back</p>"));
    }

    // --- Stage 3: notes changed by outside tools (the four cells of the design table) ---
    void outsideChange_isReportedOnceAndOwnSavesAreNot()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, QStringLiteral("<p>mine</p>"), QStringLiteral("default")));
        m_store->trackOpen(id);
        QSignalSpy changed(m_store, &NoteStore::noteChangedOutside), removed(m_store, &NoteStore::noteRemovedOutside);

        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 0);                       // nothing happened

        QVERIFY(m_store->save(id, QStringLiteral("<p>own edit</p>"), QStringLiteral("default")));
        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 0);                       // our own save is not an outside change

        QVERIFY(writeFile(m_store->noteDir(id) + "/note.html", "<p>from the sync client, longer</p>"));
        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 1);
        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 1);                       // reported once per change
        QCOMPARE(removed.count(), 0);
    }

    void outsideColorChange_isReported()
    {
        const QString id = m_store->createNote();
        m_store->trackOpen(id);
        QSignalSpy changed(m_store, &NoteStore::noteChangedOutside);
        QVERIFY(writeFile(m_store->noteDir(id) + "/meta.json", "{\"color\":\"blue\"}"));
        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 1);
    }

    void outsideRemoval_isReportedOnce()
    {
        const QString id = m_store->createNote();
        m_store->trackOpen(id);
        QSignalSpy removed(m_store, &NoteStore::noteRemovedOutside);
        QVERIFY(QDir(m_store->noteDir(id)).removeRecursively());
        m_store->checkOpenNotes();
        m_store->checkOpenNotes();
        QCOMPARE(removed.count(), 1);
    }

    void untrackedNotesAreNotReported()
    {
        const QString id = m_store->createNote();
        m_store->trackOpen(id);
        m_store->untrack(id);
        QSignalSpy changed(m_store, &NoteStore::noteChangedOutside);
        QVERIFY(writeFile(m_store->noteDir(id) + "/note.html", "<p>x</p>"));
        m_store->checkOpenNotes();
        QCOMPARE(changed.count(), 0);
    }

    // --- Stage 3: folder change and window state ---
    void changeFolder_switchesAndRemembersWithoutTouchingTheOldFolder()
    {
        const QString id = m_store->createNote();
        const QString other = m_tmp.path() + QStringLiteral("/other");
        QVERIFY(QDir().mkpath(other));
        QVERIFY(m_store->changeFolder(other));
        QCOMPARE(m_store->folder(), other);
        QVERIFY(m_store->noteIds().isEmpty());              // empty folder, notes are not copied
        QVERIFY(QFileInfo(m_root + '/' + id).isDir());     // the old folder is untouched
        QSettings check(m_tmp.path() + QStringLiteral("/s.ini"), QSettings::IniFormat);
        QCOMPARE(check.value("notesFolder").toString(), other);
    }

    void changeFolder_toMissingFolderFailsAndKeepsTheSetting()
    {
        QVERIFY(m_store->changeFolder(m_root));
        QVERIFY(!m_store->changeFolder(m_tmp.path() + QStringLiteral("/nope")));
        QSettings check(m_tmp.path() + QStringLiteral("/s.ini"), QSettings::IniFormat);
        QCOMPARE(check.value("notesFolder").toString(), m_root);
    }

    void windowState_storesSizeAlways_positionOnlyWhenAsked_andIsRemovedOnDeleteForever()
    {
        const QString id = m_store->createNote();
        m_store->saveWindowState(id, 300, 200, 10, 20, false);
        QVariantMap s = m_store->windowState(id);
        QCOMPARE(s.value("width").toInt(), 300);
        QVERIFY(!s.contains("x"));
        m_store->saveWindowState(id, 310, 210, 11, 21, true);
        s = m_store->windowState(id);
        QCOMPARE(s.value("x").toInt(), 11);
        QVERIFY(m_store->moveToBin(id));
        QVERIFY(m_store->deleteForever(id));
        QVERIFY(m_store->windowState(id).isEmpty());
    }

    // --- symlinks planted in a shared or synced notes folder ---
    void symlinkedNoteFolderIsNotListedOrLoaded()
    {
        const QString outside = m_tmp.path() + "/outside";
        QVERIFY(QDir().mkpath(outside));
        QVERIFY(writeFile(outside + "/note.html", "<p>from elsewhere</p>"));
        const QString fake = "0123abcd-0123-4abc-8abc-0123456789ab";
        QVERIFY(QFile::link(outside, m_root + '/' + fake));
        QVERIFY(!m_store->noteIds().contains(fake));
        QVERIFY(!m_store->load(fake).value("ok").toBool());
        QVERIFY(!m_store->moveToBin(fake));
    }

    void symlinkedBinEntryIsNeverDeletedThrough()
    {
        const QString victim = m_tmp.path() + "/victim";
        QVERIFY(QDir().mkpath(victim));
        QVERIFY(writeFile(victim + "/precious.txt", "keep"));
        QVERIFY(QDir().mkpath(m_root + "/.bin"));
        const QString fake = "0123abcd-0123-4abc-8abc-0123456789ab";
        QVERIFY(QFile::link(victim, m_root + "/.bin/" + fake));
        QVERIFY(!m_store->binNames().contains(fake));
        QVERIFY(!m_store->deleteForever(fake));
        m_store->emptyBin();
        QVERIFY(QFileInfo::exists(victim + "/precious.txt"));
        QVERIFY(!m_store->restoreFromBin(fake));
    }

    void saveRefusesToWriteThroughASymlinkedNoteFile()
    {
        const QString id = m_store->createNote();
        const QString target = m_tmp.path() + "/target.txt";
        QVERIFY(writeFile(target, "do not touch"));
        const QString file = m_store->noteDir(id) + "/note.html";
        QVERIFY(QFile::remove(file));
        QVERIFY(QFile::link(target, file));
        QSignalSpy spy(m_store, &NoteStore::saveFailed);
        QVERIFY(!m_store->save(id, QStringLiteral("<p>overwrite</p>"), QStringLiteral("default")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(readAll(target), QByteArray("do not touch"));
    }

    void imageIsNotWrittenThroughASymlinkedImagesFolder()
    {
        const QString id = m_store->createNote();
        const QString elsewhere = m_tmp.path() + "/elsewhere";
        QVERIFY(QDir().mkpath(elsewhere));
        QVERIFY(QDir(m_store->noteDir(id) + "/images").removeRecursively());
        QVERIFY(QFile::link(elsewhere, m_store->noteDir(id) + "/images"));
        QImage img(4, 4, QImage::Format_ARGB32);
        img.fill(Qt::red);
        QSignalSpy spy(m_store, &NoteStore::imageRejected);
        QVERIFY(m_store->addImage(id, img).isEmpty());
        QCOMPARE(spy.count(), 1);
        QVERIFY(QDir(elsewhere).entryList(QDir::Files).isEmpty());
    }

    // .bin itself is the link: entries inside it are inside the link target, not inside the notes folder.
    void symlinkedBinFolderIsNeverUsed()
    {
        const QString victim = m_tmp.path() + "/victim";
        const QString fake = "0123abcd-0123-4abc-8abc-0123456789ab";
        QVERIFY(QDir().mkpath(victim + '/' + fake));
        QVERIFY(writeFile(victim + '/' + fake + "/note.html", "<p>precious</p>"));
        QVERIFY(QFile::link(victim, m_root + "/.bin"));

        QVERIFY(m_store->binNames().isEmpty());                 // not listed, so not loaded or searched
        QVERIFY(!m_store->deleteForever(fake));
        m_store->emptyBin();
        QVERIFY(QFileInfo::exists(victim + '/' + fake + "/note.html"));

        const QString id = m_store->createNote();               // moving a note into the link target is refused
        QVERIFY(!m_store->moveToBin(id));
        QVERIFY(QFileInfo(m_store->noteDir(id)).isDir());
        QVERIFY(QDir(victim).entryList(QDir::Dirs | QDir::NoDotAndDotDot) == QStringList{fake});
        QVERIFY(!m_store->restoreFromBin(fake));
        QVERIFY(!m_store->loadAt(m_store->binDir(fake)).value("ok").toBool());
    }

    // The notes folder may itself be reached through a symlink, for example a folder that was moved and linked.
    void aSymlinkedNotesFolderStillWorksWithItsBin()
    {
        const QString real = m_tmp.path() + "/real";
        const QString link = m_tmp.path() + "/link";
        QVERIFY(QDir().mkpath(real));
        QVERIFY(QFile::link(real, link));
        QVERIFY(m_store->setFolder(link));
        const QString id = m_store->createNote();
        QVERIFY(!id.isEmpty());
        QVERIFY(m_store->moveToBin(id));
        QCOMPARE(m_store->binNames(), QStringList{id});
        QVERIFY(m_store->deleteForever(id));
        QVERIFY(m_store->binNames().isEmpty());
    }

private:
    static QByteArray readAll(const QString &p)
    {
        QFile f(p);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }
    static bool writeFile(const QString &p, const QByteArray &data)
    {
        QFile f(p);
        return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
    }

    QTemporaryDir m_tmp;
    QString m_root;
    NoteStore *m_store = nullptr;
};

QTEST_MAIN(TstNoteStore)
#include "tst_notestore.moc"
