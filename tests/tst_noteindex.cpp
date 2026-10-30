#include "NoteIndex.h"
#include "NoteStore.h"

#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QSettings>
#include <QTest>

class TstNoteIndex : public QObject
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

    void titleRule()
    {
        QCOMPARE(NoteIndex::titleFrom(QString()), QStringLiteral("Untitled"));
        QCOMPARE(NoteIndex::titleFrom(QStringLiteral("\n  \n")), QStringLiteral("Untitled"));
        QCOMPARE(NoteIndex::titleFrom(QStringLiteral("\n  first line \nsecond")), QStringLiteral("first line"));
        QCOMPARE(NoteIndex::titleFrom(QString(80, QLatin1Char('a'))).size(), 60);
    }

    void filterIsCaseInsensitiveSubstringOverTitleAndText()
    {
        add("<p>Shopping list</p><p>milk and eggs</p>");
        add("<p>Meeting notes</p>");
        NoteIndex idx(m_store, false);
        idx.rescan();
        QCOMPARE(idx.rowCount(), 2);
        idx.setFilter(QStringLiteral("EGGS"));
        QCOMPARE(idx.rowCount(), 1);
        idx.setFilter(QStringLiteral("meet"));
        QCOMPARE(idx.rowCount(), 1);
        idx.setFilter(QStringLiteral("nothing"));
        QCOMPARE(idx.rowCount(), 0);
        idx.setFilter(QString());
        QCOMPARE(idx.rowCount(), 2);
    }

    void neverOpenedNoteIsFoundBySearch()
    {
        const QString id = add("<p>needle in a haystack</p>");
        Q_UNUSED(id)
        NoteIndex idx(m_store, false);
        idx.rescan();
        idx.setFilter(QStringLiteral("needle"));
        QCOMPARE(idx.rowCount(), 1);
    }

    void binIsNotInTheMainListOrSearch()
    {
        const QString keep = add("<p>keep me</p>");
        const QString gone = add("<p>binned secret</p>");
        Q_UNUSED(keep)
        QVERIFY(m_store->moveToBin(gone));
        NoteIndex main(m_store, false), bin(m_store, true);
        main.rescan();
        bin.rescan();
        main.setFilter(QStringLiteral("secret"));
        QCOMPARE(main.rowCount(), 0);
        QCOMPARE(bin.rowCount(), 1);
    }

    void sortedNewestFirst()
    {
        const QString oldId = add("<p>old</p>");
        const QString newId = add("<p>new</p>");
        setMtime(oldId, QDateTime::currentDateTime().addDays(-2));
        setMtime(newId, QDateTime::currentDateTime());
        NoteIndex idx(m_store, false);
        idx.rescan();
        QCOMPARE(idx.data(idx.index(0), NoteIndex::IdRole).toString(), newId);
        QCOMPARE(idx.data(idx.index(1), NoteIndex::IdRole).toString(), oldId);
    }

    void unreadableNoteIsAnErrorRowAndSortsLast()
    {
        const QString good = add("<p>good</p>");
        const QString bad = add("<p>bad</p>");
        QVERIFY(QFile::remove(m_store->noteDir(bad) + "/note.html"));
        NoteIndex idx(m_store, false);
        idx.rescan();
        QCOMPARE(idx.rowCount(), 2);
        QCOMPARE(idx.data(idx.index(0), NoteIndex::IdRole).toString(), good);
        QVERIFY(idx.data(idx.index(1), NoteIndex::IsErrorRole).toBool());
    }

    void rescanAddsChangesAndRemovesOneRowWithoutReset()
    {
        const QString a = add("<p>alpha</p>");
        const QString b = add("<p>beta</p>");
        setMtime(a, QDateTime::currentDateTime().addDays(-1));
        NoteIndex idx(m_store, false);
        idx.rescan();
        QSignalSpy reset(&idx, &QAbstractItemModel::modelReset);
        QSignalSpy inserted(&idx, &QAbstractItemModel::rowsInserted);
        QSignalSpy removed(&idx, &QAbstractItemModel::rowsRemoved);
        QSignalSpy changed(&idx, &QAbstractItemModel::dataChanged);

        const QString c = add("<p>gamma</p>");
        idx.rescan();
        QCOMPARE(inserted.count(), 1);

        QVERIFY(m_store->save(b, QStringLiteral("<p>beta edited</p>"), QStringLiteral("blue")));
        setMtime(b, QDateTime::currentDateTime().addSecs(60));
        idx.rescan();
        QVERIFY(changed.count() >= 1);

        QVERIFY(m_store->moveToBin(c));
        idx.rescan();
        QCOMPARE(removed.count(), 1);
        QCOMPARE(reset.count(), 0);
        QCOMPARE(idx.rowCount(), 2);
    }

    void thousandNotesLoad()
    {
        QElapsedTimer t;
        t.start();
        for (int i = 0; i < 1000; ++i)
            add(QStringLiteral("<p>note %1</p>").arg(i).toUtf8());
        NoteIndex idx(m_store, false);
        t.restart();
        idx.rescan();
        qInfo() << "first rescan of 1000 notes:" << t.elapsed() << "ms";
        QCOMPARE(idx.rowCount(), 1000);
        t.restart();
        idx.rescan();
        qInfo() << "second rescan (no changes):" << t.elapsed() << "ms";
        idx.setFilter(QStringLiteral("note 999"));
        QCOMPARE(idx.rowCount(), 1);
    }

private:
    QString add(const QByteArray &html)
    {
        const QString id = m_store->createNote();
        m_store->save(id, QString::fromUtf8(html), QStringLiteral("default"));
        return id;
    }
    void setMtime(const QString &id, const QDateTime &when)
    {
        QFile f(m_store->noteDir(id) + "/note.html");
        QVERIFY(f.open(QIODevice::ReadWrite));
        QVERIFY(f.setFileTime(when, QFileDevice::FileModificationTime));
    }

    QTemporaryDir m_tmp;
    QString m_root;
    NoteStore *m_store = nullptr;
};

QTEST_MAIN(TstNoteIndex)
#include "tst_noteindex.moc"
