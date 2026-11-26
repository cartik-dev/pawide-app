#include "editor/SyntaxHighlighter.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QTextLayout>
#include <QtTest>

using namespace paw;

namespace {

// Formats painted on one block.
QList<QTextLayout::FormatRange> formats(const QTextBlock &block)
{
    return block.layout()->formats();
}

bool hasForeground(const QList<QTextLayout::FormatRange> &ranges, int start, int len, const QColor &color)
{
    for (const auto &r : ranges) {
        if (r.start == start && r.length == len && r.format.foreground().color() == color)
            return true;
    }
    return false;
}

} // namespace

class TstHighlighter : public QObject {
    Q_OBJECT

private slots:
    void pythonKeywordsStringsComments()
    {
        QTextDocument doc;
        SyntaxHighlighter h(&doc);
        h.setLanguage(QStringLiteral("python"));
        doc.setPlainText(QStringLiteral("def run(x):\n    s = \"hello\"  # note\n"));
        QTest::qWait(0); // let the highlighter process the blocks
        const QTextBlock l1 = doc.findBlockByNumber(0);
        QVERIFY(hasForeground(formats(l1), 0, 3, SyntaxHighlighter::keywordColor()));
        const QTextBlock l2 = doc.findBlockByNumber(1);
        QVERIFY(hasForeground(formats(l2), 8, 7, SyntaxHighlighter::stringColor()));
        QVERIFY(hasForeground(formats(l2), 17, 6, SyntaxHighlighter::commentColor()));
    }

    void jsonStringsAndNumbers()
    {
        QTextDocument doc;
        SyntaxHighlighter h(&doc);
        h.setLanguage(QStringLiteral("json"));
        doc.setPlainText(QStringLiteral("{\"n\": 42}"));
        QTest::qWait(0);
        const QTextBlock b = doc.findBlockByNumber(0);
        QVERIFY(hasForeground(formats(b), 5, 2, SyntaxHighlighter::numberColor()));
    }

    void blockCommentSpansBlocks()
    {
        QTextDocument doc;
        SyntaxHighlighter h(&doc);
        h.setLanguage(QStringLiteral("cpp"));
        doc.setPlainText(QStringLiteral("int x; /* start\nstill comment */ int y;\n"));
        QTest::qWait(0);
        const QTextBlock l1 = doc.findBlockByNumber(0);
        QVERIFY(hasForeground(formats(l1), 0, 3, SyntaxHighlighter::keywordColor()));
        QVERIFY(hasForeground(formats(l1), 7, 8, SyntaxHighlighter::commentColor()));
        const QTextBlock l2 = doc.findBlockByNumber(1);
        QVERIFY(hasForeground(formats(l2), 0, 16, SyntaxHighlighter::commentColor()));
        QVERIFY(hasForeground(formats(l2), 17, 3, SyntaxHighlighter::keywordColor()));
    }

    void languageSwitchRehighlights()
    {
        QTextDocument doc;
        SyntaxHighlighter h(&doc);
        doc.setPlainText(QStringLiteral("#include \"a.h\"\n"));
        h.setLanguage(QStringLiteral("cpp"));
        QTest::qWait(0);
        const QTextBlock b = doc.findBlockByNumber(0);
        QVERIFY(hasForeground(formats(b), 0, 8, SyntaxHighlighter::preprocessorColor()));
    }
};

QTEST_MAIN(TstHighlighter)
#include "tst_highlighter.moc"
