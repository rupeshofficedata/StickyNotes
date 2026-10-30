#include "AppLifecycle.h"
#include "NoteStore.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QSettings>
#include <QTest>

// One test per row of the lifecycle table in the design, plus the quit sequence and the cascade.
class TstAppLifecycle : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(m_tmp.isValid());
        m_store = new NoteStore(m_tmp.path(), new QSettings(m_tmp.path() + QStringLiteral("/s.ini"), QSettings::IniFormat, this), this);
        QVERIFY(m_store->setFolder(m_tmp.path()));
    }
    void cleanup()
    {
        delete m_store;
        m_store = nullptr;
        QDir(m_tmp.path()).removeRecursively();
    }

    // Row: close primary window, tray present
    void trayPresent_closePrimary_hidesAndKeepsRunning()
    {
        AppLifecycle lc(m_store, true);
        QSignalSpy hide(&lc, &AppLifecycle::hidePrimaryRequested), quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("primary", {});
        lc.primaryCloseRequested();
        QCOMPARE(hide.count(), 1);
        lc.windowHidden("primary", {});
        QCOMPARE(quit.count(), 0);
    }

    // Row: close primary window, no tray
    void noTray_closePrimary_withVisibleNote_hides()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy hide(&lc, &AppLifecycle::hidePrimaryRequested), quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("primary", {});
        lc.windowShown("note", "a");
        lc.primaryCloseRequested();
        QCOMPARE(hide.count(), 1);
        lc.windowHidden("primary", {});
        QCOMPARE(quit.count(), 0);
    }

    void noTray_closePrimary_withNoNote_quits()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy hide(&lc, &AppLifecycle::hidePrimaryRequested), quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("primary", {});
        lc.primaryCloseRequested();
        QCOMPARE(hide.count(), 0);
        QCOMPARE(quit.count(), 1);
    }

    // Row: close a note window
    void trayPresent_closeLastNote_keepsRunning()
    {
        AppLifecycle lc(m_store, true);
        QSignalSpy quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("note", "a");
        lc.windowHidden("note", "a");
        QCOMPARE(quit.count(), 0);
    }

    void noTray_closeLastNote_primaryHidden_quits()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("note", "a");
        lc.windowHidden("note", "a");
        QCOMPARE(quit.count(), 1);
    }

    void noTray_closeLastNote_primaryVisible_keepsRunning()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("primary", {});
        lc.windowShown("note", "a");
        lc.windowHidden("note", "a");
        QCOMPARE(quit.count(), 0);
    }

    void noTray_closeOneOfTwoNotes_keepsRunning()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy quit(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("note", "a");
        lc.windowShown("note", "b");
        lc.windowHidden("note", "a");
        QCOMPARE(quit.count(), 0);
    }

    // Row: list button, relaunch, tray click
    void showPrimary_emitsRequest()
    {
        AppLifecycle lc(m_store, true);
        QSignalSpy show(&lc, &AppLifecycle::showPrimaryRequested);
        lc.showPrimary();
        QCOMPARE(show.count(), 1);
    }

    void newNoteCreatesAndRequestsOpen()
    {
        AppLifecycle lc(m_store, true);
        QSignalSpy open(&lc, &AppLifecycle::openNoteRequested);
        lc.newNote();
        QCOMPARE(open.count(), 1);
        QVERIFY(m_store->noteIds().contains(open.first().first().toString()));
    }

    // Row: Quit
    void quit_flushesAndApproves()
    {
        AppLifecycle lc(m_store, true);
        QSignalSpy flush(m_store, &NoteStore::flushRequested), ok(&lc, &AppLifecycle::quitApproved);
        lc.quit();
        QCOMPARE(flush.count(), 1);
        QCOMPARE(ok.count(), 1);
    }

    void quit_withFailedSave_isBlocked_untilForced()
    {
        const QString id = m_store->createNote();
        const QString dir = m_store->noteDir(id);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::ExeOwner);
        if (QFileInfo(dir).isWritable())
            QSKIP("running as a user that ignores directory permissions");
        QVERIFY(!m_store->save(id, "<p>x</p>", "default"));

        AppLifecycle lc(m_store, true);
        QSignalSpy blocked(&lc, &AppLifecycle::quitBlocked), ok(&lc, &AppLifecycle::quitApproved);
        lc.quit();
        QCOMPARE(blocked.count(), 1);
        QCOMPARE(blocked.first().first().toStringList(), QStringList{id});
        QCOMPARE(ok.count(), 0);
        lc.quit(true);   // Discard and quit, or SIGTERM
        QCOMPARE(ok.count(), 1);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

    void quit_afterRetrySucceeds_isApproved()
    {
        const QString id = m_store->createNote();
        const QString dir = m_store->noteDir(id);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::ExeOwner);
        if (QFileInfo(dir).isWritable())
            QSKIP("running as a user that ignores directory permissions");
        QVERIFY(!m_store->save(id, "<p>x</p>", "default"));
        AppLifecycle lc(m_store, true);
        QSignalSpy blocked(&lc, &AppLifecycle::quitBlocked), ok(&lc, &AppLifecycle::quitApproved);
        lc.quit();
        QCOMPARE(blocked.count(), 1);
        QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
        // A window saves when asked to flush. The retry then succeeds and clears the failure.
        connect(m_store, &NoteStore::flushRequested, this, [&] { m_store->save(id, "<p>x</p>", "default"); });
        lc.quit();
        QCOMPARE(ok.count(), 1);
    }

    void quit_onlyApprovesOnce()
    {
        AppLifecycle lc(m_store, false);
        QSignalSpy ok(&lc, &AppLifecycle::quitApproved);
        lc.windowShown("note", "a");
        lc.windowHidden("note", "a");   // quits
        lc.quit();
        lc.quit(true);
        QCOMPARE(ok.count(), 1);
    }

    void cascadeStepsAndWrapsAfterEight()
    {
        AppLifecycle lc(m_store, true);
        QCOMPARE(lc.nextCascadePosition(), QPoint(80, 80));
        QCOMPARE(lc.nextCascadePosition(), QPoint(112, 112));
        for (int i = 2; i < 8; ++i)
            lc.nextCascadePosition();
        QCOMPARE(lc.nextCascadePosition(), QPoint(80, 80));   // 9th note wraps
    }

private:
    QTemporaryDir m_tmp;
    NoteStore *m_store = nullptr;
};

QTEST_MAIN(TstAppLifecycle)
#include "tst_applifecycle.moc"
