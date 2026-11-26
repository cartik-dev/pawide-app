#include "SearchPanel.h"

#include <QCheckBox>
#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace paw {

namespace {
const QStringList kSkipDirs = {QStringLiteral(".git"), QStringLiteral("node_modules"), QStringLiteral("target"),
                               QStringLiteral("build"), QStringLiteral("dist"), QStringLiteral("__pycache__"),
                               QStringLiteral(".venv"), QStringLiteral("venv"), QStringLiteral("vendor")};
constexpr int kMaxResults = 5000;
constexpr qint64 kMaxBytes = 2 * 1024 * 1024; // do not scan files above 2 MB
constexpr int kMaxLineLength = 240;

QString displayText(QString line)
{
    line.remove(QChar(0x9)); // tabs render fine, but shorten very long lines
    if (line.size() > kMaxLineLength)
        line = line.left(kMaxLineLength) + QStringLiteral("…");
    return line;
}
} // namespace

SearchPanel::SearchPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("SEARCH"), this);
    title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
    layout->addWidget(title);

    m_query = new QLineEdit(this);
    m_query->setPlaceholderText(tr("Search in project"));
    m_query->setClearButtonEnabled(true);
    connect(m_query, &QLineEdit::returnPressed, this, &SearchPanel::runSearch);
    layout->addWidget(m_query);

    auto *opts = new QHBoxLayout;
    m_case = new QCheckBox(tr("Match Case"), this);
    m_whole = new QCheckBox(tr("Whole Word"), this);
    layout->addWidget(m_case);
    layout->addWidget(m_whole);
    opts->addStretch(1);
    layout->addLayout(opts);

    m_summary = new QLabel(tr("Type a query and press Enter."), this);
    m_summary->setObjectName(QStringLiteral("muted"));
    m_summary->setWordWrap(true);
    layout->addWidget(m_summary);

    m_results = new QTreeWidget(this);
    m_results->setHeaderHidden(true);
    m_results->setUniformRowHeights(true);
    m_results->setColumnCount(1);
    m_results->header()->setStretchLastSection(true);
    connect(m_results, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *it, int) {
        if (!it || it->parent() == nullptr)
            return;
        const QVariant line = it->data(0, Qt::UserRole + 1);
        if (line.isValid())
            emit resultActivated(it->data(0, Qt::UserRole).toString(), line.toInt());
    });
    connect(m_results, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *it, int) {
        if (it && it->parent() != nullptr) {
            const QVariant line = it->data(0, Qt::UserRole + 1);
            if (line.isValid())
                emit resultActivated(it->data(0, Qt::UserRole).toString(), line.toInt());
        }
    });
    layout->addWidget(m_results, 1);
}

void SearchPanel::setRootPath(const QString &path)
{
    m_root = path;
    m_results->clear();
    m_summary->setText(path.isEmpty() ? tr("Open a folder to search in it.")
                                      : tr("Search in %1").arg(QDir(path).dirName()));
}

void SearchPanel::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    m_query->setFocus();
}

void SearchPanel::runSearch()
{
    if (m_busy) // the scan is synchronous; re-entry is only possible while blocked
        return;
    if (m_root.isEmpty()) {
        m_summary->setText(tr("Open a folder first."));
        return;
    }
    const QString needle = m_query->text();
    if (needle.isEmpty())
        return;

    m_busy = true;
    m_summary->setText(tr("Searching…"));
    m_results->clear();

    const bool caseSensitive = m_case->isChecked();
    const bool wholeWord = m_whole->isChecked();

    int files = 0, matches = 0;
    bool truncated = false;
    const QDir rootDir(m_root);
    QDirIterator it(m_root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (matches >= kMaxResults) {
            truncated = true;
            break;
        }
        const QString full = it.next();
        const QString rel = rootDir.relativeFilePath(full);
        bool skip = false;
        for (const QString &d : kSkipDirs)
            if (rel.startsWith(d + QLatin1Char('/')) || rel.contains(QLatin1Char('/') + d + QLatin1Char('/')))
                skip = true;
        if (skip)
            continue;
        QFileInfo fi(full);
        if (fi.size() > kMaxBytes)
            continue;

        QFile f(full);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);
        ts.setGenerateByteOrderMark(false);

        int lineNo = 0;
        QTreeWidgetItem *fileItem = nullptr;
        while (!ts.atEnd()) {
            ++lineNo;
            const QString line = ts.readLine();
            if (line.contains(QLatin1Char('\0')))
                break; // binary file
            int from = 0;
            bool firstOnLine = true;
            for (;;) {
                const QString hay = caseSensitive ? line : line.toLower();
                const QString what = caseSensitive ? needle : needle.toLower();
                int idx = hay.indexOf(what, from);
                if (idx < 0)
                    break;
                from = idx + needle.size();
                if (wholeWord) {
                    const auto isWord = [hay](int p) {
                        return p < 0 || p >= hay.size() || !(hay.at(p).isLetterOrNumber() || hay.at(p) == QLatin1Char('_'));
                    };
                    if (!isWord(idx - 1) || !isWord(idx + needle.size()))
                        continue;
                }
                if (firstOnLine) {
                    fileItem = new QTreeWidgetItem(m_results);
                    fileItem->setText(0, rel);
                    fileItem->setData(0, Qt::UserRole, full);
                    fileItem->setForeground(0, QColor(0xa7, 0x8b, 0xfa));
                    ++files;
                    firstOnLine = false;
                }
                auto *hit = new QTreeWidgetItem(fileItem);
                hit->setText(0, QStringLiteral("%1:  %2").arg(lineNo).arg(displayText(line.trimmed())));
                hit->setData(0, Qt::UserRole, full);
                hit->setData(0, Qt::UserRole + 1, lineNo);
                ++matches;
                from = idx + needle.size();
                if (matches >= kMaxResults || from > hay.size())
                    break;
            }
            if (matches >= kMaxResults) {
                truncated = true;
                break;
            }
        }
    }
    m_busy = false;
    m_summary->setText(tr("%1 match(es) in %2 file(s)%3")
                           .arg(matches)
                           .arg(files)
                           .arg(truncated ? QStringLiteral(" (showing the first %1)").arg(kMaxResults) : QString()));
    if (m_results->topLevelItemCount() > 0)
        m_results->expandAll();
    if (truncated)
        emit logMessage(tr("Search stopped after %1 matches.").arg(kMaxResults));
}

} // namespace paw
