#include "RichTextBridge.h"
#include "NoteStore.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextList>
#include <QTextTable>

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

namespace {

bool isNoteImage(const QString &name)
{
    static const QRegularExpression ok(
        QStringLiteral("^images/[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\\.png$"));
    return ok.match(name).hasMatch();
}

// Background images are the only resource a format can load besides an image fragment.
void scrubFrame(QTextFrame *frame)
{
    QTextFrameFormat ff = frame->frameFormat();
    if (ff.hasProperty(QTextFormat::BackgroundImageUrl)) {
        ff.clearProperty(QTextFormat::BackgroundImageUrl);
        frame->setFrameFormat(ff);
    }
    if (auto *table = qobject_cast<QTextTable *>(frame)) {
        for (int r = 0; r < table->rows(); ++r) {
            for (int c = 0; c < table->columns(); ++c) {
                QTextTableCell cell = table->cellAt(r, c);
                QTextCharFormat cf = cell.format();
                if (cf.hasProperty(QTextFormat::BackgroundImageUrl)) {
                    cf.clearProperty(QTextFormat::BackgroundImageUrl);
                    cell.setFormat(cf);
                }
            }
        }
    }
    const auto children = frame->childFrames();
    for (QTextFrame *child : children)
        scrubFrame(child);
}

} // namespace

QString RichTextBridge::sanitizeHtml(const QString &html)
{
    // A plain QTextDocument never loads anything. Qt's own parser decides which tags and CSS become an
    // image or a background image, so there is no regex to get around (single quotes, duplicate src, CSS url()).
    QTextDocument doc;
    doc.setHtml(html);
    scrubFrame(doc.rootFrame());

    struct Range { int pos; int len; };
    QVector<Range> badImages, badBackgrounds;
    QVector<int> badBlocks;
    for (QTextBlock b = doc.begin(); b.isValid(); b = b.next()) {
        if (b.blockFormat().hasProperty(QTextFormat::BackgroundImageUrl))
            badBlocks.append(b.position());
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment f = it.fragment();
            const QTextCharFormat cf = f.charFormat();
            if (cf.isImageFormat() && !isNoteImage(cf.toImageFormat().name()))
                badImages.append({f.position(), f.length()});
            else if (cf.hasProperty(QTextFormat::BackgroundImageUrl))
                badBackgrounds.append({f.position(), f.length()});
        }
    }
    // Apply after the walk, so no fragment iterator is used while the document changes.
    // Format changes keep positions; removals run last, from the end of the document.
    for (int pos : badBlocks) {
        QTextCursor c(&doc);
        c.setPosition(pos);
        QTextBlockFormat bf = c.blockFormat();
        bf.clearProperty(QTextFormat::BackgroundImageUrl);
        c.setBlockFormat(bf);
    }
    for (const Range &r : badBackgrounds) {
        QTextCursor c(&doc);
        c.setPosition(r.pos);
        c.setPosition(r.pos + r.len, QTextCursor::KeepAnchor);
        QTextCharFormat cf = c.charFormat();
        cf.clearProperty(QTextFormat::BackgroundImageUrl);
        c.setCharFormat(cf);
    }
    for (auto it = badImages.crbegin(); it != badImages.crend(); ++it) {
        QTextCursor c(&doc);
        c.setPosition(it->pos);
        c.setPosition(it->pos + it->len, QTextCursor::KeepAnchor);
        c.removeSelectedText();
    }
    return doc.toHtml();
}
