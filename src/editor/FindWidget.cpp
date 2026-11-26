#include "FindWidget.h"

#include "editor/CodeEditor.h"
#include "ui/Theme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

namespace paw {

namespace {
constexpr int kMaxHighlightedMatches = 2000;

QToolButton *toolButton(const QString &text, const QString &tip, bool checkable)
{
    auto *b = new QToolButton;
    b->setText(text);
    b->setToolTip(tip);
    b->setCheckable(checkable);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}
} // namespace

FindWidget::FindWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("findBar"));
    setStyleSheet(QStringLiteral(
        "QWidget#findBar { background: %1; border: 1px solid %2; border-radius: 6px; }"
        "QWidget#findBar QLabel { color: %3; }")
                      .arg(theme::bgSideBar.name(), theme::border.name(), theme::textMuted.name()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(6);

    // Row 1: [Aa] [find ] [count] [prev][next] [x]
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(4);
    m_caseBtn = toolButton(QStringLiteral("Aa"), tr("Match Case"), true);
    m_caseBtn->setStyleSheet(m_caseBtn->styleSheet() + QStringLiteral(
        "QToolButton { border: 1px solid %1; border-radius: 4px; padding: 2px 6px; }"
        "QToolButton:checked { background: %2; color: #ffffff; }")
                                 .arg(theme::border.name(), theme::accent.name()));
    connect(m_caseBtn, &QToolButton::toggled, this, [this](bool on) {
        m_caseSensitive = on;
        runSearch(true);
    });
    row1->addWidget(m_caseBtn);

    m_find = new QLineEdit;
    m_find->setPlaceholderText(tr("Find"));
    m_find->setClearButtonEnabled(true);
    m_find->setMinimumWidth(220);
    connect(m_find, &QLineEdit::textChanged, this, [this] { runSearch(false); });
    connect(m_find, &QLineEdit::returnPressed, this, [this] { findNext(false); });
    m_find->installEventFilter(this);
    row1->addWidget(m_find, 1);

    m_count = new QLabel(this);
    m_count->setMinimumWidth(70);
    m_count->setAlignment(Qt::AlignCenter);
    row1->addWidget(m_count);

    auto *prev = toolButton(tr("\u2191"), tr("Previous Match (Shift+Enter)"), false);
    prev->setStyleSheet(QStringLiteral("QToolButton { border: 1px solid %1; border-radius: 4px; padding: 2px 7px; }").arg(theme::border.name()));
    connect(prev, &QToolButton::clicked, this, [this] { findNext(true); });
    auto *next = toolButton(tr("\u2193"), tr("Next Match (Enter)"), false);
    next->setStyleSheet(prev->styleSheet());
    connect(next, &QToolButton::clicked, this, [this] { findNext(false); });
    row1->addWidget(prev);
    row1->addWidget(next);

    auto *close = toolButton(tr("\u2715"), tr("Close (Escape)"), false);
    close->setStyleSheet(prev->styleSheet());
    connect(close, &QToolButton::clicked, this, &FindWidget::closeBar);
    row1->addWidget(close);
    root->addLayout(row1);

    // Row 2: [replace ] [Replace] [Replace All]
    m_replaceRow = new QWidget(this);
    auto *row2 = new QHBoxLayout(m_replaceRow);
    row2->setContentsMargins(0, 0, 0, 0);
    row2->setSpacing(4);
    m_replace = new QLineEdit;
    m_replace->setPlaceholderText(tr("Replace"));
    m_replace->setMinimumWidth(220);
    m_replace->installEventFilter(this);
    connect(m_replace, &QLineEdit::returnPressed, this, [this] { replaceOne(); });
    row2->addWidget(m_replace, 1);
    auto *repBtn = new QPushButton(tr("Replace"));
    connect(repBtn, &QPushButton::clicked, this, [this] { replaceOne(); });
    auto *repAllBtn = new QPushButton(tr("Replace All"));
    connect(repAllBtn, &QPushButton::clicked, this, [this] { replaceAll(); });
    row2->addWidget(repBtn);
    row2->addWidget(repAllBtn);
    root->addWidget(m_replaceRow);
    m_replaceRow->setVisible(false);

    adjustSize();
}

void FindWidget::attach(CodeEditor *editor)
{
    if (m_editor == editor)
        return;
    m_editor = editor;
    if (isVisible())
        runSearch(false);
}

void FindWidget::open(CodeEditor *editor, bool withReplace, const QString &initialText)
{
    m_editor = editor;
    m_replaceRow->setVisible(withReplace);
    if (!initialText.isEmpty() && initialText.length() < 200 && !initialText.contains(QLatin1Char('\n')))
        m_find->setText(initialText); // pre-fill with the selection
    show();
    raise();
    adjustSize();
    runSearch(false);
    m_find->setFocus();
    m_find->selectAll();
}

void FindWidget::closeBar()
{
    hide();
    if (m_editor) {
        m_editor->setSearchSelections({});
        m_editor->setFocus();
    }
}

bool FindWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape) {
            closeBar();
            return true;
        }
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            findNext(ke->modifiers() & Qt::ShiftModifier);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FindWidget::runSearch(bool keepCurrent)
{
    updateMatches();
    if (!keepCurrent && m_editor)
        findNext(false);
}

void FindWidget::updateMatches()
{
    m_matches = 0;
    QList<QTextEdit::ExtraSelection> sels;
    if (!m_editor) {
        if (m_count)
            m_count->setText(tr("No results"));
        return;
    }
    const QString needle = m_find->text();
    if (!needle.isEmpty()) {
        QTextDocument *doc = m_editor->document();
        QTextDocument::FindFlags flags;
        if (m_caseSensitive)
            flags |= QTextDocument::FindCaseSensitively;
        QTextCursor cur(doc);
        cur.movePosition(QTextCursor::Start);
        QTextCursor hit = doc->find(needle, cur, flags);
        while (hit.hasSelection() && sels.size() < kMaxHighlightedMatches) {
            QTextEdit::ExtraSelection s;
            s.format.setBackground(QColor(0x8b, 0x5c, 0xf6, 70));
            s.cursor = hit;
            sels.push_back(s);
            hit = doc->find(needle, hit, flags);
        }
        if (hit.hasSelection() && sels.size() == kMaxHighlightedMatches)
            ++m_matches; // counted but not highlighted
    }
    m_editor->setSearchSelections(sels);
    m_matches += sels.size();
    if (m_find->text().isEmpty())
        m_count->setText(QString());
    else if (sels.size() == kMaxHighlightedMatches)
        m_count->setText(tr("%1+ matches").arg(kMaxHighlightedMatches));
    else if (m_matches == 0)
        m_count->setText(tr("No results"));
    else
        m_count->setText(tr("%n match(es)", "", m_matches));
}

void FindWidget::findNext(bool backward)
{
    if (!m_editor || m_find->text().isEmpty())
        return;
    QTextDocument::FindFlags flags;
    if (m_caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    if (backward)
        flags |= QTextDocument::FindBackward;
    QTextCursor found = m_editor->document()->find(m_find->text(), m_editor->textCursor(), flags);
    if (!found.hasSelection()) { // wrap around
        QTextCursor wrap(m_editor->document());
        wrap.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
        found = m_editor->document()->find(m_find->text(), wrap, flags);
    }
    if (found.hasSelection()) {
        m_editor->setTextCursor(found);
        m_editor->ensureCursorVisible();
    }
}

void FindWidget::replaceOne()
{
    if (!m_editor)
        return;
    QTextCursor c = m_editor->textCursor();
    const QString needle = m_find->text();
    const QString hay = m_caseSensitive ? c.selectedText() : c.selectedText().toLower();
    if (c.hasSelection() && hay == (m_caseSensitive ? needle : needle.toLower())) {
        c.insertText(m_replace->text());
    }
    findNext(false);
    updateMatches();
}

void FindWidget::replaceAll()
{
    if (!m_editor)
        return;
    const QString needle = m_find->text();
    if (needle.isEmpty())
        return;
    QTextDocument::FindFlags flags;
    if (m_caseSensitive)
        flags |= QTextDocument::FindCaseSensitively;
    QTextCursor group(m_editor->document());
    group.beginEditBlock();
    QTextCursor hit = m_editor->document()->find(needle, group, flags);
    int n = 0;
    while (hit.hasSelection() && n < 100000) {
        hit.insertText(m_replace->text());
        ++n;
        hit = m_editor->document()->find(needle, hit, flags);
    }
    group.endEditBlock();
    updateMatches();
}

} // namespace paw
