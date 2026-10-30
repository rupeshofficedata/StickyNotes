#include "RichTextBridge.h"
#include "NoteStore.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextList>

RichTextBridge::RichTextBridge(QObject *parent) : QObject(parent) {}

void RichTextBridge::setDocument(QQuickTextDocument *doc)
{
    if (doc == m_document)
        return;
    if (textDocument())
        disconnect(textDocument(), nullptr, this, nullptr);
    m_document = doc;
    if (textDocument())
        connect(textDocument(), &QTextDocument::contentsChanged, this, &RichTextBridge::contentsChanged);
    emit documentChanged();
}

void RichTextBridge::setStore(NoteStore *store)
{
    if (store == m_store)
        return;
    m_store = store;
    emit storeChanged();
}

void RichTextBridge::setNoteId(const QString &id)
{
    if (id == m_noteId)
        return;
    m_noteId = id;
    emit noteIdChanged();
}

void RichTextBridge::setCursorPosition(int p)
{
    m_cursor = p;
    emit formatChanged();
}

void RichTextBridge::setSelectionStart(int p)
{
    m_selStart = p;
    emit formatChanged();
}

void RichTextBridge::setSelectionEnd(int p)
{
    m_selEnd = p;
    emit formatChanged();
}

QTextDocument *RichTextBridge::textDocument() const
{
    return m_document ? m_document->textDocument() : nullptr;
}

QTextCursor RichTextBridge::selectionCursor() const
{
    QTextCursor c(textDocument());
    if (m_selStart != m_selEnd) {
        c.setPosition(m_selStart);
        c.setPosition(m_selEnd, QTextCursor::KeepAnchor);
    } else {
        c.setPosition(m_cursor);
    }
    return c;
}

void RichTextBridge::mergeFormat(const QTextCharFormat &format)
{
    if (!textDocument())
        return;
    QTextCursor c = selectionCursor();
    if (!c.hasSelection())
        c.select(QTextCursor::WordUnderCursor);
    c.mergeCharFormat(format);
    emit formatChanged();
}

bool RichTextBridge::bold() const
{
    return textDocument() && selectionCursor().charFormat().fontWeight() > QFont::Normal;
}

bool RichTextBridge::italic() const
{
    return textDocument() && selectionCursor().charFormat().fontItalic();
}

bool RichTextBridge::underline() const
{
    return textDocument() && selectionCursor().charFormat().fontUnderline();
}

bool RichTextBridge::strike() const
{
    return textDocument() && selectionCursor().charFormat().fontStrikeOut();
}

bool RichTextBridge::bullet() const
{
    return textDocument() && selectionCursor().currentList() != nullptr;
}

void RichTextBridge::toggleBold()
{
    QTextCharFormat f;
    f.setFontWeight(bold() ? QFont::Normal : QFont::Bold);
    mergeFormat(f);
}

void RichTextBridge::toggleItalic()
{
    QTextCharFormat f;
    f.setFontItalic(!italic());
    mergeFormat(f);
}

void RichTextBridge::toggleUnderline()
{
    QTextCharFormat f;
    f.setFontUnderline(!underline());
    mergeFormat(f);
}

void RichTextBridge::toggleStrike()
{
    QTextCharFormat f;
    f.setFontStrikeOut(!strike());
    mergeFormat(f);
}

void RichTextBridge::toggleBullet()
{
    if (!textDocument())
        return;
    QTextCursor c = selectionCursor();
    if (QTextList *list = c.currentList()) {
        list->remove(c.block());
        QTextBlockFormat bf = c.blockFormat();
        bf.setIndent(0);
        c.setBlockFormat(bf);
    } else {
        c.createList(QTextListFormat::ListDisc);
    }
    emit formatChanged();
}

void RichTextBridge::insertImageAt(const QString &relativePath, const QSize &size)
{
    // Show the image no wider than a small note window; the file keeps its full size.
    const int w = qMin(size.width(), 240);
    QTextImageFormat f;
    f.setName(relativePath);
    f.setWidth(w);
    f.setHeight(static_cast<qreal>(size.height()) * w / qMax(1, size.width()));
    QTextCursor c(textDocument());
    c.setPosition(m_cursor);
    c.insertImage(f);
}

bool RichTextBridge::insertImage(const QImage &image)
{
    if (!textDocument() || !m_store)
        return false;
    const QString rel = m_store->addImage(m_noteId, image);
    if (rel.isEmpty())
        return false;
    insertImageAt(rel, image.size());
    return true;
}

bool RichTextBridge::insertImageFile(const QUrl &url)
{
    if (!url.isLocalFile() || !m_store)
        return false;
    // addImageFile applies the 10 MB limit to the source file and ignores files that are not images.
    const QString rel = m_store->addImageFile(m_noteId, url.toLocalFile());
    if (rel.isEmpty() || !textDocument())
        return false;
    insertImageAt(rel, QImage(m_store->noteDir(m_noteId) + QLatin1Char('/') + rel).size());
    return true;
}

void RichTextBridge::pasteFromClipboard()
{
    const QMimeData *mime = QGuiApplication::clipboard()->mimeData();
    if (!mime || !textDocument())
        return;
    if (mime->hasImage()) {
        insertImage(qvariant_cast<QImage>(mime->imageData()));
    } else if (mime->hasUrls()) {
        for (const QUrl &u : mime->urls())
            insertImageFile(u);
    } else if (mime->hasText()) {
        // Plain text only, so foreground colors from other apps cannot make text unreadable on a dark note.
        QTextCursor c = selectionCursor();
        c.insertText(mime->text());
    }
}

QString RichTextBridge::html() const
{
    return textDocument() ? textDocument()->toHtml() : QString();
}

QString RichTextBridge::sanitizeHtml(const QString &html)
{
    static const QRegularExpression img(QStringLiteral("<img\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression src(QStringLiteral("\\bsrc\\s*=\\s*\"([^\"]*)\""),
                                        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression ok(
        QStringLiteral("^images/[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\.png$"));

    QString out;
    qsizetype last = 0;
    auto it = img.globalMatch(html);
    while (it.hasNext()) {
        const auto m = it.next();
        out += html.mid(last, m.capturedStart() - last);
        last = m.capturedEnd();
        const auto s = src.match(m.captured());
        if (s.hasMatch() && ok.match(s.captured(1)).hasMatch())
            out += m.captured();
        // else: drop the tag
    }
    out += html.mid(last);
    return out;
}
