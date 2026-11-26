#pragma once
#include <QColor>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QTextDocument;

namespace paw {

// Regex-based syntax highlighting for 30+ languages, styled with the
// black / white / violet identity of Paw IDE. Block comments and Python
// triple-quoted strings span multiple blocks via the highlighter state.
class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    explicit SyntaxHighlighter(QTextDocument *doc);

    void setLanguage(const QString &languageId);
    QString languageId() const { return m_language; }

    // Shared palette (also used by tests).
    static QColor keywordColor() { return {0x8b, 0x5c, 0xf6}; }
    static QColor typeColor() { return {0xa7, 0x8b, 0xfa}; }
    static QColor stringColor() { return {0x7e, 0xe7, 0x87}; }
    static QColor numberColor() { return {0xf0, 0xa3, 0x5e}; }
    static QColor commentColor() { return {0x6b, 0x6b, 0x78}; }
    static QColor functionColor() { return {0x79, 0xc0, 0xff}; }
    static QColor preprocessorColor() { return {0xff, 0xa6, 0x57}; }
    static QColor tagColor() { return {0xf8, 0x71, 0x71}; }

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void rebuild();
    void addRule(const QString &pattern, const QTextCharFormat &fmt);

    QString m_language;
    QList<Rule> m_rules;
    QRegularExpression m_lineComment; // empty pattern = disabled
    QString m_blockStart;
    QString m_blockEnd;
};

} // namespace paw
