#pragma once
#include "core/FileUtils.h"
#include "core/Language.h"

#include <QPlainTextEdit>

namespace paw {

class LineNumberArea;

class CodeEditor : public QPlainTextEdit {
    Q_OBJECT
public:
    explicit CodeEditor(QWidget *parent = nullptr);

    QString filePath() const { return m_path; }
    void setFilePath(const QString &p);
    bool isUntitled() const { return m_path.isEmpty(); }
    QString displayName() const;
    QString encoding() const { return m_encoding; }
    void setEncoding(const QString &e) { m_encoding = e; emit fileInfoChanged(); }
    LineEnding lineEnding() const { return m_eol; }
    void setLineEnding(LineEnding e);
    bool hasMixedLineEndings() const { return m_mixedEol; }
    LanguageInfo language() const;

    void loadFrom(const TextFile &f);
    bool save(QString *error);
    bool saveAs(const QString &path, QString *error);

    void applySettings();
    void goToLine(int line); // 1-based
    int currentLine() const { return textCursor().blockNumber() + 1; }
    int currentColumn() const { return textCursor().positionInBlock() + 1; }

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

signals:
    void fileInfoChanged();
    void cursorInfoChanged(int line, int column);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateExtraSelections();

private:
    QString indentUnit() const;
    void indentSelection(bool outdent);
    bool handleAutoPair(QKeyEvent *e);
    void insertNewlineWithIndent();
    int matchingBracket(int pos) const;

    LineNumberArea *m_lineArea;
    QString m_path;
    QString m_encoding = QStringLiteral("UTF-8");
    LineEnding m_eol = LineEnding::LF;
    bool m_mixedEol = false;
};

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *e) : QWidget(e), m_editor(e) {}
    QSize sizeHint() const override { return QSize(m_editor->lineNumberAreaWidth(), 0); }

protected:
    void paintEvent(QPaintEvent *event) override { m_editor->lineNumberAreaPaintEvent(event); }

private:
    CodeEditor *m_editor;
};

} // namespace paw
