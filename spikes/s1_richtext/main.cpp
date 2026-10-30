// Spike S1: does rich text round-trip through QTextDocument::toHtml / setHtml?
// Pass: second save byte-identical and formatting survives. Throwaway code.
#include <QGuiApplication>
#include <QImage>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextList>
#include <QDebug>
#include <QDir>
#include <QTextStream>
static QTextStream &out() { static QTextStream s(stdout); return s << Qt::endl, s; }

static QString html(QTextDocument &d) { return d.toHtml(); }

struct Flags { bool bold=false, italic=false, underline=false, strike=false, list=false, image=false; };

static Flags scan(const QTextDocument &d)
{
    Flags f;
    for (QTextBlock b = d.begin(); b.isValid(); b = b.next()) {
        if (b.textList()) f.list = true;
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat cf = it.fragment().charFormat();
            if (cf.isImageFormat()) f.image = true;
            if (cf.fontWeight() >= QFont::Bold) f.bold = true;
            if (cf.fontItalic()) f.italic = true;
            if (cf.fontUnderline()) f.underline = true;
            if (cf.fontStrikeOut()) f.strike = true;
        }
    }
    return f;
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    QDir().mkpath("images");
    QImage img(32, 32, QImage::Format_ARGB32); img.fill(Qt::red);
    img.save("images/test.png");

    QTextDocument doc;
    QTextCursor c(&doc);
    QTextCharFormat plain, b, i, u, s;
    b.setFontWeight(QFont::Bold); i.setFontItalic(true);
    u.setFontUnderline(true); s.setFontStrikeOut(true);
    c.insertText("plain ", plain); c.insertText("bold ", b); c.insertText("italic ", i);
    c.insertText("underline ", u); c.insertText("strike", s);
    c.insertBlock();
    c.insertList(QTextListFormat::ListDisc);
    c.insertText("item one"); c.insertBlock(); c.insertText("item two");
    c.insertBlock(); // leaves list? start a fresh plain block below
    QTextBlockFormat bf; c.setBlockFormat(bf);
    QTextImageFormat imf; imf.setName("images/test.png"); imf.setWidth(32); imf.setHeight(32);
    c.insertImage(imf);

    const Flags before = scan(doc);
    const QString h1 = html(doc);

    QTextDocument d2; d2.setHtml(h1); const QString h2 = html(d2);
    QTextDocument d3; d3.setHtml(h2); const QString h3 = html(d3);
    const Flags after = scan(d3);

    out() << "save1==save2:" << (h1 == h2) << " save2==save3:" << (h2 == h3);
    auto flags = [](const char *n, const Flags &f) {
        out() << n << "bold" << f.bold << "italic" << f.italic << "underline" << f.underline
                          << "strike" << f.strike << "list" << f.list << "image" << f.image; };
    flags("before:", before); flags("after: ", after);
    const bool formatOk = after.bold && after.italic && after.underline && after.strike && after.list && after.image;
    out() << "HTML formatting survives:" << formatOk;
    out() << "HTML stable after first reload (save2==save3):" << (h2 == h3);
    out() << "HTML identical on first reload (save1==save2):" << (h1 == h2);

    // Markdown alternative
    const QString md = doc.toMarkdown();
    QTextDocument dm; dm.setMarkdown(md);
    const Flags mf = scan(dm);
    out() << "markdown:" << md.simplified();
    flags("md after:", mf);

    QFile("s1_save1.html").open(QIODevice::WriteOnly);
    { QFile f("s1_save1.html"); f.open(QIODevice::WriteOnly); f.write(h1.toUtf8()); }
    { QFile f("s1_save2.html"); f.open(QIODevice::WriteOnly); f.write(h2.toUtf8()); }
    return 0;
}
