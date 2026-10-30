#include "AppLifecycle.h"
#include "NoteIndex.h"
#include "NoteStore.h"
#include "RichTextBridge.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

// Drives the real NoteWindow.qml headless: type, autosave, close, reopen, outside edits.
class TstNoteWindowUi : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Build a temporary "StickyNotes" QML module from the real source files.
        QVERIFY(m_modules.isValid());
        const QString dir = m_modules.path() + QStringLiteral("/StickyNotes");
        QVERIFY(QDir().mkpath(dir));
        for (const char *f : {"Theme.qml", "NoteWindow.qml", "ErrorBanner.qml"})
            QVERIFY(QFile::copy(QStringLiteral(QML_SOURCE_DIR "/") + QLatin1String(f), dir + '/' + QLatin1String(f)));
        QFile qmldir(dir + QStringLiteral("/qmldir"));
        QVERIFY(qmldir.open(QIODevice::WriteOnly));
        qmldir.write("module StickyNotes\nsingleton Theme 1.0 Theme.qml\nNoteWindow 1.0 NoteWindow.qml\nErrorBanner 1.0 ErrorBanner.qml\n");
        qmldir.close();

        m_settings = new QSettings(m_modules.path() + QStringLiteral("/s.ini"), QSettings::IniFormat, this);
        m_store = new NoteStore(m_modules.path(), m_settings, this);
        m_index = new NoteIndex(m_store, false, this);
        m_life = new AppLifecycle(m_store, true, this);
        qmlRegisterSingletonInstance("StickyNotes", 1, 0, "NoteStore", m_store);
        qmlRegisterSingletonInstance("StickyNotes", 1, 0, "NoteIndex", m_index);
        qmlRegisterSingletonInstance("StickyNotes", 1, 0, "Lifecycle", m_life);
        qmlRegisterType<RichTextBridge>("StickyNotes", 1, 0, "RichTextBridge");
        // Singletons registered by instance may only be used from one engine, so all tests share one.
        m_engine = new QQmlEngine(this);
        m_engine->addImportPath(m_modules.path());
    }

    void init()
    {
        QVERIFY(m_notes.isValid());
        QVERIFY(m_store->setFolder(m_notes.path()));
    }
    void cleanup()
    {
        qDeleteAll(m_windows);
        m_windows.clear();
        const auto ids = m_store->noteIds();
        for (const QString &id : ids)
            QDir(m_store->noteDir(id)).removeRecursively();
    }

    void typingIsAutosavedAndSurvivesCloseAndReopen()
    {
        const QString id = m_store->createNote();
        QQuickWindow *w = open(id);
        setText(w, "<p>hello <b>world</b></p>");
        QTRY_VERIFY_WITH_TIMEOUT(noteHtml(id).contains("hello"), 3000);
        QVERIFY(noteHtml(id).contains("world"));

        close(w);
        QQuickWindow *w2 = open(id);
        QVERIFY(text(w2).contains("hello"));
        QVERIFY(text(w2).contains("world"));
    }

    void colorChoiceIsSaved()
    {
        const QString id = m_store->createNote();
        QQuickWindow *w = open(id);
        w->setProperty("colorToken", "blue");
        w->setProperty("dirty", true);
        QMetaObject::invokeMethod(w, "save", Q_ARG(QVariant, QVariant(true)));
        QCOMPARE(m_store->load(id).value("colorToken").toString(), QStringLiteral("blue"));
    }

    void closeFlushesEditsWithoutWaitingForTheTimer()
    {
        const QString id = m_store->createNote();
        QQuickWindow *w = open(id);
        setText(w, "<p>quick close</p>");
        close(w);   // before the 400 ms timer fires
        QVERIFY(noteHtml(id).contains("quick close"));
    }

    void outsideChange_onACleanNote_reloadsSilently()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, "<p>before</p>", "default"));
        QQuickWindow *w = open(id);
        QVERIFY(text(w).contains("before"));
        writeNote(id, "<p>after, written by a sync client</p>");
        m_store->checkOpenNotes();
        QVERIFY(text(w).contains("after, written by a sync client"));
        QVERIFY(banner(w)->property("message").toString().isEmpty());
    }

    void outsideChange_onADirtyNote_showsBannerAndKeepMineOverwrites()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, "<p>before</p>", "default"));
        QQuickWindow *w = open(id);
        setText(w, "<p>my unsaved edit</p>");
        writeNote(id, "<p>outside version that is clearly longer</p>");
        m_store->checkOpenNotes();
        QVERIFY(!banner(w)->property("message").toString().isEmpty());
        QVERIFY(text(w).contains("my unsaved edit"));       // nothing was thrown away

        QMetaObject::invokeMethod(banner(w), "actionTriggered", Q_ARG(int, 1), Q_ARG(QString, "Keep mine"));
        close(w);
        QVERIFY(noteHtml(id).contains("my unsaved edit"));   // Keep mine: our save overwrote the file
    }

    void outsideChange_onADirtyNote_reloadDiscardsEdits()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, "<p>before</p>", "default"));
        QQuickWindow *w = open(id);
        setText(w, "<p>my unsaved edit</p>");
        writeNote(id, "<p>outside version that is clearly longer</p>");
        m_store->checkOpenNotes();
        QMetaObject::invokeMethod(banner(w), "actionTriggered", Q_ARG(int, 0), Q_ARG(QString, "Reload"));
        QVERIFY(text(w).contains("outside version"));
        QVERIFY(!text(w).contains("my unsaved edit"));
    }

    void outsideRemoval_onADirtyNote_offersRecreate()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, "<p>before</p>", "default"));
        QQuickWindow *w = open(id);
        setText(w, "<p>still here</p>");
        QVERIFY(QDir(m_store->noteDir(id)).removeRecursively());
        m_store->checkOpenNotes();
        QVERIFY(!banner(w)->property("message").toString().isEmpty());
        QMetaObject::invokeMethod(banner(w), "actionTriggered", Q_ARG(int, 0), Q_ARG(QString, "Recreate note"));
        QVERIFY(noteHtml(id).contains("still here"));
    }

    void outsideRemoval_onACleanNote_closesTheWindow()
    {
        const QString id = m_store->createNote();
        QVERIFY(m_store->save(id, "<p>before</p>", "default"));
        QQuickWindow *w = open(id);
        QVERIFY(w->isVisible());
        QVERIFY(QDir(m_store->noteDir(id)).removeRecursively());
        m_store->checkOpenNotes();
        QVERIFY(!w->isVisible());
    }

    void imageDropAndRemoteImageFilter()
    {
        const QString id = m_store->createNote();
        const QString evil = "<p>x<img src=\"https://evil.example/a.png\">y</p>";
        QVERIFY(m_store->save(id, evil, "default"));
        QQuickWindow *w = open(id);
        QVERIFY(!text(w).contains("evil.example"));
    }

private:
    QQuickWindow *open(const QString &id)
    {
        QQmlComponent c(m_engine);
        c.setData(QByteArray("import QtQuick\nimport StickyNotes\nNoteWindow { noteId: \"") + id.toUtf8() + "\" }", QUrl());
        auto *w = qobject_cast<QQuickWindow *>(c.create());
        if (!w)
            qFatal("cannot create NoteWindow: %s", qPrintable(c.errorString()));
        m_windows.append(w);
        w->show();
        return w;
    }
    void close(QQuickWindow *w)
    {
        w->close();
        QTest::qWait(10);
    }
    static QObject *edit(QQuickWindow *w) { return w->findChild<QObject *>("edit"); }
    static QObject *banner(QQuickWindow *w) { return w->findChild<QObject *>("banner"); }
    static void setText(QQuickWindow *w, const QString &html)
    {
        QObject *e = edit(w);
        QVERIFY2(e, "edit not found");
        e->setProperty("text", html);
    }
    static QString text(QQuickWindow *w)
    {
        QObject *e = edit(w);
        return e ? e->property("text").toString() : QString();
    }
    QString noteHtml(const QString &id) const { return m_store->load(id).value("html").toString(); }
    void writeNote(const QString &id, const QByteArray &html)
    {
        QFile f(m_store->noteDir(id) + "/note.html");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(html);
    }

    QTemporaryDir m_modules, m_notes;
    QSettings *m_settings = nullptr;
    NoteStore *m_store = nullptr;
    NoteIndex *m_index = nullptr;
    AppLifecycle *m_life = nullptr;
    QQmlEngine *m_engine = nullptr;
    QList<QQuickWindow *> m_windows;
};

QTEST_MAIN(TstNoteWindowUi)
#include "tst_notewindow_ui.moc"
