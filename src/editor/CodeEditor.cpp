#include "CodeEditor.h"

#include "editor/SyntaxHighlighter.h"
#include "settings/Settings.h"
#include "ui/Theme.h"

#include <QFileInfo>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QPainter>
#include <QTextBlock>

namespace paw {

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent), m_lineArea(new LineNumberArea(this))
{
    setFrameShape(QFrame::NoFrame);
    setUndoRedoEnabled(true);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &CodeEditor::updateExtraSelections);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this,
            [this] { emit cursorInfoChanged(currentLine(), currentColumn()); });
    applySettings();
    m_highlighter = new SyntaxHighlighter(document());
    updateLineNumberAreaWidth();
    updateExtraSelections();
}

void CodeEditor::updateHighlighter()
{
    if (m_highlighter)
        m_highlighter->setLanguage(language().id);
}

void CodeEditor::setFilePath(const QString &p)
{
    m_path = p;
    updateHighlighter();
    emit fileInfoChanged();
}

QString CodeEditor::displayName() const
{
    return m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).fileName();
}

LanguageInfo CodeEditor::language() const
{
    return m_path.isEmpty() ? LanguageInfo{QStringLiteral("plaintext"), QStringLiteral("Plain Text")}
                            : Language::forFile(m_path);
}

void CodeEditor::setLineEnding(LineEnding e)
{
    m_eol = e;
    m_mixedEol = false;
    document()->setModified(true);
    emit fileInfoChanged();
}

void CodeEditor::loadFrom(const TextFile &f)
{
    m_encoding = f.encoding;
    m_eol = f.eol;
    m_mixedEol = f.mixedEol;
    setPlainText(f.text);
    document()->setModified(false);
    document()->clearUndoRedoStacks();
    emit fileInfoChanged();
}

bool CodeEditor::save(QString *error)
{
    if (m_path.isEmpty()) {
        if (error)
            *error = tr("The document has no file name.");
        return false;
    }
    return saveAs(m_path, error);
}

bool CodeEditor::saveAs(const QString &path, QString *error)
{
    QString enc = m_encoding;
    if (!FileUtils::writeText(path, toPlainText(), enc, m_eol, error))
        return false;
    m_encoding = enc;
    m_path = path;
    m_mixedEol = false;
    document()->setModified(false);
    updateHighlighter();
    emit fileInfoChanged();
    return true;
}

void CodeEditor::applySettings()
{
    auto &s = Settings::instance();
    QFont f = s.fontFamily().isEmpty() ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : QFont(s.fontFamily());
    if (s.fontFamily().isEmpty()) {
        const QStringList preferred = {QStringLiteral("Cascadia Code"), QStringLiteral("JetBrains Mono"),
                                       QStringLiteral("Consolas"), QStringLiteral("DejaVu Sans Mono")};
        const QStringList families = QFontDatabase::families();
        for (const QString &p : preferred) {
            if (families.contains(p)) {
                f = QFont(p);
                break;
            }
        }
    }
    f.setStyleHint(QFont::Monospace);
    f.setPointSize(s.fontSize());
    setFont(f);
    setTabStopDistance(QFontMetricsF(f).horizontalAdvance(QLatin1Char(' ')) * s.tabSize());
    setLineWrapMode(s.wordWrap() ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    updateLineNumberAreaWidth();
}

void CodeEditor::goToLine(int line)
{
    const int n = qBound(1, line, blockCount());
    QTextCursor c(document()->findBlockByNumber(n - 1));
    setTextCursor(c);
    centerCursor();
    setFocus();
}

// ---- line numbers -------------------------------------------------------

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 16 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * qMax(3, digits);
}

void CodeEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineArea->scroll(0, dy);
    else
        m_lineArea->update(0, rect.y(), m_lineArea->width(), rect.height());
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    const QRect cr = contentsRect();
    m_lineArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineArea);
    painter.fillRect(event->rect(), theme::bgEditor);
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    const int current = textCursor().blockNumber();
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(blockNumber == current ? theme::text : QColor(0x5a, 0x5a, 0x66));
            painter.drawText(0, top, m_lineArea->width() - 8, fontMetrics().height(), Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---- highlighting of current line + matching brackets ----------------------

int CodeEditor::matchingBracket(int pos) const
{
    const QTextDocument *doc = document();
    if (pos < 0 || pos >= doc->characterCount() - 1)
        return -1;
    const QChar c = doc->characterAt(pos);
    static const QString open = QStringLiteral("([{"), close = QStringLiteral(")]}");
    int idx = open.indexOf(c);
    int dir = 1;
    QChar partner;
    if (idx >= 0) {
        partner = close.at(idx);
    } else {
        idx = close.indexOf(c);
        if (idx < 0)
            return -1;
        partner = open.at(idx);
        dir = -1;
    }
    int depth = 0;
    const int limit = 20000;
    for (int i = pos, n = 0; i >= 0 && i < doc->characterCount() && n < limit; i += dir, ++n) {
        const QChar ch = doc->characterAt(i);
        if (ch == c)
            ++depth;
        else if (ch == partner && --depth == 0)
            return i;
    }
    return -1;
}

void CodeEditor::updateExtraSelections()
{
    QList<QTextEdit::ExtraSelection> sels;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection line;
        line.format.setBackground(theme::bgLineHighlight);
        line.format.setProperty(QTextFormat::FullWidthSelection, true);
        line.cursor = textCursor();
        line.cursor.clearSelection();
        sels.append(line);
    }
    sels.append(m_searchSels);
    sels.append(m_issueSels);
    const int pos = textCursor().position();
    for (int p : {pos, pos - 1}) {
        const int m = matchingBracket(p);
        if (m >= 0) {
            for (int q : {p, m}) {
                QTextEdit::ExtraSelection s;
                s.format.setBackground(QColor(0x8b, 0x5c, 0xf6, 90));
                s.cursor = QTextCursor(document());
                s.cursor.setPosition(q);
                s.cursor.setPosition(q + 1, QTextCursor::KeepAnchor);
                sels.append(s);
            }
            break;
        }
    }
    setExtraSelections(sels);
}

// ---- typing behaviour ----------------------------------------------------

QString CodeEditor::indentUnit() const
{
    auto &s = Settings::instance();
    return s.insertSpaces() ? QString(s.tabSize(), QLatin1Char(' ')) : QStringLiteral("\t");
}

void CodeEditor::indentSelection(bool outdent)
{
    QTextCursor c = textCursor();
    const int start = c.selectionStart(), end = c.selectionEnd();
    QTextCursor sc(document());
    sc.setPosition(start);
    const int firstBlock = sc.blockNumber();
    sc.setPosition(end);
    int lastBlock = sc.blockNumber();
    if (end > start && sc.atBlockStart())
        --lastBlock;
    const QString unit = indentUnit();
    c.beginEditBlock();
    for (int b = firstBlock; b <= lastBlock; ++b) {
        QTextCursor lc(document()->findBlockByNumber(b));
        if (outdent) {
            const QString t = lc.block().text();
            int remove = 0;
            if (t.startsWith(QLatin1Char('\t'))) {
                remove = 1;
            } else {
                while (remove < unit.size() && remove < t.size() && t.at(remove) == QLatin1Char(' '))
                    ++remove;
            }
            lc.movePosition(QTextCursor::StartOfBlock);
            lc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, remove);
            lc.removeSelectedText();
        } else if (!lc.block().text().isEmpty() || firstBlock == lastBlock) {
            lc.insertText(unit);
        }
    }
    c.endEditBlock();
}

void CodeEditor::insertNewlineWithIndent()
{
    QTextCursor c = textCursor();
    const QString line = c.block().text();
    const int posInBlock = c.positionInBlock();
    QString indent;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' ') || ch == QLatin1Char('\t'))
            indent += ch;
        else
            break;
    }
    indent.truncate(qMin<qsizetype>(indent.size(), posInBlock));
    const QString before = line.left(posInBlock).trimmed();
    const QChar prev = before.isEmpty() ? QChar() : before.at(before.size() - 1);
    const QChar next = posInBlock < line.size() ? line.at(posInBlock) : QChar();
    const bool python = language().id == QLatin1String("python");
    const bool opens = prev == QLatin1Char('{') || prev == QLatin1Char('(') || prev == QLatin1Char('[')
                       || (python && prev == QLatin1Char(':'));

    c.beginEditBlock();
    c.removeSelectedText();
    if (opens && ((prev == QLatin1Char('{') && next == QLatin1Char('}')) || (prev == QLatin1Char('(') && next == QLatin1Char(')'))
                  || (prev == QLatin1Char('[') && next == QLatin1Char(']')))) {
        c.insertText(QStringLiteral("\n") + indent + indentUnit());
        const int mid = c.position();
        c.insertText(QStringLiteral("\n") + indent);
        c.setPosition(mid);
    } else {
        c.insertText(QStringLiteral("\n") + indent + (opens ? indentUnit() : QString()));
    }
    c.endEditBlock();
    setTextCursor(c);
    ensureCursorVisible();
}

bool CodeEditor::handleAutoPair(QKeyEvent *e)
{
    if (e->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        return false;
    const QString t = e->text();
    if (t.size() != 1)
        return false;
    const QChar ch = t.at(0);
    static const QString openers = QStringLiteral("([{\"'`");
    static const QString closers = QStringLiteral(")]}\"'`");
    QTextCursor c = textCursor();
    const QString line = c.block().text();
    const int pos = c.positionInBlock();
    const QChar next = pos < line.size() ? line.at(pos) : QChar();
    const QChar prev = pos > 0 ? line.at(pos - 1) : QChar();

    // Type over an existing closing character.
    if (closers.contains(ch) && !c.hasSelection() && next == ch) {
        c.movePosition(QTextCursor::NextCharacter);
        setTextCursor(c);
        return true;
    }
    if (!openers.contains(ch))
        return false;
    const bool isQuote = (ch == QLatin1Char('"') || ch == QLatin1Char('\'') || ch == QLatin1Char('`'));
    if (c.hasSelection()) { // surround selection
        const QString sel = c.selectedText();
        const QChar close = closers.at(openers.indexOf(ch));
        c.insertText(QString(ch) + sel + close);
        return true;
    }
    if (isQuote && (prev.isLetterOrNumber() || prev == QLatin1Char('\\')))
        return false; // apostrophes, escapes
    if (!next.isNull() && !next.isSpace() && !closers.contains(next) && next != QLatin1Char(';') && next != QLatin1Char(','))
        return false;
    const QChar close = closers.at(openers.indexOf(ch));
    c.beginEditBlock();
    c.insertText(QString(ch) + close);
    c.movePosition(QTextCursor::PreviousCharacter);
    c.endEditBlock();
    setTextCursor(c);
    return true;
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    if (isReadOnly()) {
        QPlainTextEdit::keyPressEvent(e);
        return;
    }
    switch (e->key()) {
    case Qt::Key_Tab:
        if (!(e->modifiers() & Qt::ControlModifier)) {
            const QTextCursor c = textCursor();
            if (c.hasSelection() && document()->findBlock(c.selectionStart()) != document()->findBlock(c.selectionEnd()))
                indentSelection(false);
            else
                textCursor().insertText(indentUnit());
            return;
        }
        break;
    case Qt::Key_Backtab:
        indentSelection(true);
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (!(e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
            insertNewlineWithIndent();
            return;
        }
        break;
    case Qt::Key_Backspace:
        if (!textCursor().hasSelection() && !(e->modifiers() & Qt::ControlModifier)) {
            QTextCursor c = textCursor();
            const QString line = c.block().text();
            const int pos = c.positionInBlock();
            if (pos > 0 && pos < line.size()) {
                const QString pair = QString(line.at(pos - 1)) + line.at(pos);
                if (pair == QLatin1String("()") || pair == QLatin1String("[]") || pair == QLatin1String("{}")
                    || pair == QLatin1String("\"\"") || pair == QLatin1String("''") || pair == QLatin1String("``")) {
                    c.beginEditBlock();
                    c.deleteChar();
                    c.deletePreviousChar();
                    c.endEditBlock();
                    return;
                }
            }
        }
        break;
    default:
        break;
    }
    if (handleAutoPair(e))
        return;
    QPlainTextEdit::keyPressEvent(e);
}

} // namespace paw
