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
