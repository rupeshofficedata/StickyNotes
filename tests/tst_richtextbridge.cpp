#include "NoteStore.h"
#include "RichTextBridge.h"

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTemporaryDir>
#include <QSettings>
#include <QTest>
#include <QTextBlock>
#include <QTextDocument>

class TstRichTextBridge : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        QVERIFY(m_tmp.isValid());
        m_store = new NoteStore(m_tmp.path(), new QSettings(m_tmp.path() + QStringLiteral("/s.ini"), QSettings::IniFormat, this), this);
        QVERIFY(m_store->setFolder(m_tmp.path()));
        m_id = m_store->createNote();

        QQmlComponent c(&m_engine);
        c.setData("import QtQuick\nTextEdit { textFormat: TextEdit.RichText }", QUrl());
        m_edit.reset(qobject_cast<QQuickItem *>(c.create()));
        QVERIFY2(m_edit, qPrintable(c.errorString()));
        m_bridge = new RichTextBridge(this);
        m_bridge->setStore(m_store);
        m_bridge->setNoteId(m_id);
        m_bridge->setDocument(m_edit->property("textDocument").value<QQuickTextDocument *>());
        m_edit->setProperty("text", QStringLiteral("hello world"));
    }
    void cleanup()
    {
        delete m_bridge;
        m_edit.reset();
        delete m_store;
    }

    void togglesSurviveSaveAndReload()
    {
        select(0, 5);
        m_bridge->toggleBold();
        m_bridge->toggleItalic();
        m_bridge->toggleUnderline();
        m_bridge->toggleStrike();
        QVERIFY(m_bridge->bold() && m_bridge->italic() && m_bridge->underline() && m_bridge->strike());
        const QString html = m_bridge->html();

        QTextDocument reloaded;
        reloaded.setHtml(RichTextBridge::sanitizeHtml(html));
        const QTextCharFormat f = reloaded.begin().begin().fragment().charFormat();
        QVERIFY(f.fontWeight() > QFont::Normal);
        QVERIFY(f.fontItalic());
        QVERIFY(f.fontUnderline());
        QVERIFY(f.fontStrikeOut());
    }

    void toggleTwiceRestoresPlain()
    {
        select(0, 5);
        m_bridge->toggleBold();
        m_bridge->toggleBold();
        QVERIFY(!m_bridge->bold());
    }

    void bulletTogglesOnAndOff()
    {
        m_bridge->setCursorPosition(2);
        m_bridge->toggleBullet();
        QVERIFY(m_bridge->bullet());
        m_bridge->toggleBullet();
        QVERIFY(!m_bridge->bullet());
    }

    void imageFileIsInsertedAndStored()
    {
        QImage img(20, 10, QImage::Format_ARGB32);
        img.fill(Qt::blue);
        const QString src = m_tmp.path() + "/pic.png";
        QVERIFY(img.save(src));
        QVERIFY(m_bridge->insertImageFile(QUrl::fromLocalFile(src)));
        QVERIFY(m_bridge->html().contains(QStringLiteral("images/")));
        QCOMPARE(QDir(m_store->noteDir(m_id) + "/images").entryList(QDir::Files).size(), 1);
    }

    void nonImageFileIsNotInserted()
    {
        const QString src = m_tmp.path() + "/a.txt";
        QFile f(src);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("text");
        f.close();
        QVERIFY(!m_bridge->insertImageFile(QUrl::fromLocalFile(src)));
    }

    void sanitizeKeepsOnlyNoteImages()
    {
        const QString good = QStringLiteral("images/0123abcd-0123-4abc-8abc-0123456789ab.png");
        const QString html = QStringLiteral("<p>a<img src=\"%1\">b<img src=\"https://evil.example/x.png\">c"
                                            "<img src=\"../secret.png\">d<img src=\"file:///etc/passwd\">e<img></p>")
                                 .arg(good);
        const QString out = RichTextBridge::sanitizeHtml(html);
        QVERIFY(out.contains(good));
        QVERIFY(!out.contains(QStringLiteral("evil.example")));
        QVERIFY(!out.contains(QStringLiteral("../secret")));
        QVERIFY(!out.contains(QStringLiteral("/etc/passwd")));
        QCOMPARE(out.count(QStringLiteral("<img")), 1);
    }

    // Each of these slips past a regex that only looks at <img src="...">.
    void sanitizeBlocksEveryWayToLoadARemoteResource_data()
    {
        QTest::addColumn<QString>("html");
        QTest::newRow("single-quoted src") << "<p><img src='https://evil.example/a.png'></p>";
        QTest::newRow("unquoted src") << "<p><img src=https://evil.example/a.png></p>";
        QTest::newRow("uppercase tag") << "<p><IMG SRC=\"https://evil.example/a.png\"></p>";
        QTest::newRow("duplicate src, good first")
            << "<p><img src=\"images/0123abcd-0123-4abc-8abc-0123456789ab.png\" src='https://evil.example/a.png'></p>";
        QTest::newRow("duplicate src, good last")
            << "<p><img src='https://evil.example/a.png' src=\"images/0123abcd-0123-4abc-8abc-0123456789ab.png\"></p>";
        QTest::newRow("table background attribute")
            << "<table background=\"https://evil.example/a.png\"><tr><td>x</td></tr></table>";
        QTest::newRow("cell background attribute")
            << "<table><tr><td background=\"https://evil.example/a.png\">x</td></tr></table>";
        QTest::newRow("css background-image on body")
            << "<body style=\"background-image:url(https://evil.example/a.png)\"><p>x</p></body>";
        QTest::newRow("css background-image on paragraph")
            << "<p style=\"background-image:url('https://evil.example/a.png')\">x</p>";
        QTest::newRow("css background-image on span")
            << "<p><span style=\"background-image:url(https://evil.example/a.png)\">x</span></p>";
        QTest::newRow("css background-image on table")
            << "<table style=\"background-image:url(https://evil.example/a.png)\"><tr><td>x</td></tr></table>";
        QTest::newRow("file url") << "<p><img src=\"file:///etc/passwd\"></p>";
        QTest::newRow("parent path") << "<p><img src=\"images/../../secret.png\"></p>";
    }
    void sanitizeBlocksEveryWayToLoadARemoteResource()
    {
        QFETCH(QString, html);
        const QString full = RichTextBridge::sanitizeHtml(html);
        // Qt writes a DOCTYPE line that contains a w3.org URL. Only the document itself matters.
        const QString out = full.mid(full.indexOf(QStringLiteral("<html")));
        QVERIFY2(!out.contains(QStringLiteral("evil.example")), qPrintable(out));
        QVERIFY2(!out.contains(QStringLiteral("passwd")), qPrintable(out));
        QVERIFY2(!out.contains(QStringLiteral("secret.png")), qPrintable(out));
        QVERIFY2(!out.contains(QStringLiteral("http")), qPrintable(out));
        QVERIFY2(!out.contains(QStringLiteral("background-image")), qPrintable(out));
        QVERIFY2(!out.contains(QStringLiteral("url(")), qPrintable(out));
    }

    void sanitizeKeepsTheTextAroundARemovedImage()
    {
        const QString out = RichTextBridge::sanitizeHtml(
            QStringLiteral("<p>before <img src='https://evil.example/a.png'> after</p>"));
        QVERIFY(out.contains(QStringLiteral("before")));
        QVERIFY(out.contains(QStringLiteral("after")));
    }

    void sanitizeLeavesAGoodNoteUnchangedOnSecondPass()
    {
        const QString good = QStringLiteral("images/0123abcd-0123-4abc-8abc-0123456789ab.png");
        const QString once = RichTextBridge::sanitizeHtml(
            QStringLiteral("<p><b>bold</b> <img src=\"%1\" width=\"20\" height=\"10\"></p><ul><li>item</li></ul>").arg(good));
        QVERIFY(once.contains(good));
        QVERIFY(once.contains(QStringLiteral("<li")));
        QCOMPARE(RichTextBridge::sanitizeHtml(once), once);   // stable, so a clean note is not rewritten on every open
    }

private:
    void select(int start, int end)
    {
        m_bridge->setSelectionStart(start);
        m_bridge->setSelectionEnd(end);
    }

    QTemporaryDir m_tmp;
    QQmlEngine m_engine;
    std::unique_ptr<QQuickItem> m_edit;
    NoteStore *m_store = nullptr;
    RichTextBridge *m_bridge = nullptr;
    QString m_id;
};

QTEST_MAIN(TstRichTextBridge)
#include "tst_richtextbridge.moc"
