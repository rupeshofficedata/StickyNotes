#pragma once
#include "NoteStore.h"

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QUrl>

// Formatting commands, image paste and drop for one note window. Works on the QTextDocument of a QML TextEdit.
// QML feeds it the cursor and selection; it never touches files except through NoteStore.
class RichTextBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(NoteStore *store READ store WRITE setStore NOTIFY storeChanged)
    Q_PROPERTY(QString noteId READ noteId WRITE setNoteId NOTIFY noteIdChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY formatChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY formatChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY formatChanged)
    Q_PROPERTY(bool bold READ bold NOTIFY formatChanged)
    Q_PROPERTY(bool italic READ italic NOTIFY formatChanged)
    Q_PROPERTY(bool underline READ underline NOTIFY formatChanged)
    Q_PROPERTY(bool strike READ strike NOTIFY formatChanged)
    Q_PROPERTY(bool bullet READ bullet NOTIFY formatChanged)

public:
    explicit RichTextBridge(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *doc);
    NoteStore *store() const { return m_store; }
    void setStore(NoteStore *store);
    QString noteId() const { return m_noteId; }
    void setNoteId(const QString &id);
    int cursorPosition() const { return m_cursor; }
    void setCursorPosition(int p);
    int selectionStart() const { return m_selStart; }
    void setSelectionStart(int p);
    int selectionEnd() const { return m_selEnd; }
    void setSelectionEnd(int p);

    bool bold() const;
    bool italic() const;
    bool underline() const;
    bool strike() const;
    bool bullet() const;

    Q_INVOKABLE void toggleBold();
    Q_INVOKABLE void toggleItalic();
    Q_INVOKABLE void toggleUnderline();
    Q_INVOKABLE void toggleStrike();
    Q_INVOKABLE void toggleBullet();

    // Text paste is plain text. An image on the clipboard is saved to the note and inserted.
    Q_INVOKABLE void pasteFromClipboard();
    // Returns true when an image was inserted. Non-image files are ignored.
    Q_INVOKABLE bool insertImageFile(const QUrl &url);
    bool insertImage(const QImage &image);

    Q_INVOKABLE QString html() const;
    // Keeps only <img src="images/<uuid>.png">. Remote URLs and paths outside the note folder are removed.
    Q_INVOKABLE static QString sanitizeHtml(const QString &html);

signals:
    void documentChanged();
    void storeChanged();
    void noteIdChanged();
    void formatChanged();
    void contentsChanged();

private:
    QTextDocument *textDocument() const;
    QTextCursor selectionCursor() const;
    void mergeFormat(const QTextCharFormat &format);
    void insertImageAt(const QString &relativePath, const QSize &size);

    QPointer<QQuickTextDocument> m_document;
    QPointer<NoteStore> m_store;
    QString m_noteId;
    int m_cursor = 0;
    int m_selStart = 0;
    int m_selEnd = 0;
};
