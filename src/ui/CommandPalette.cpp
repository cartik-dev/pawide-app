#include "CommandPalette.h"

#include "ui/Theme.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace paw {

CommandPalette::CommandPalette(QWidget *parent) : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint)
{
    setObjectName(QStringLiteral("palette"));
    setStyleSheet(QStringLiteral("QDialog#palette { border: 1px solid %1; }").arg(theme::accent.name()));
    auto *l = new QVBoxLayout(this);
    l->setContentsMargins(6, 6, 6, 6);
    m_edit = new QLineEdit(this);
    m_list = new QListWidget(this);
    m_list->setFocusPolicy(Qt::NoFocus);
    l->addWidget(m_edit);
    l->addWidget(m_list);
    resize(620, 380);
    m_edit->installEventFilter(this);
    connect(m_edit, &QLineEdit::textChanged, this, [this] { refilter(); });
    connect(m_list, &QListWidget::itemClicked, this, [this] { runCurrent(); });
}

bool CommandPalette::fuzzyMatch(const QString &query, const QString &text, int *score)
{
    int qi = 0, s = 0, last = -2;
    for (int i = 0; i < text.size() && qi < query.size(); ++i) {
        if (text.at(i).toLower() == query.at(qi).toLower()) {
            s += (i == last + 1) ? 3 : 1;           // consecutive characters score higher
            if (i == 0 || !text.at(i - 1).isLetterOrNumber())
                s += 2;                              // word start
            last = i;
            ++qi;
        }
    }
    if (score)
        *score = s;
    return qi == query.size();
}

void CommandPalette::showItems(const QList<PaletteItem> &items, const QString &placeholder)
{
    m_items = items;
    m_edit->setPlaceholderText(placeholder);
    m_edit->clear();
    refilter();
    if (QWidget *p = parentWidget()) {
        const QPoint c = p->mapToGlobal(QPoint(p->width() / 2 - width() / 2, 60));
        move(c);
    }
    show();
    m_edit->setFocus();
}

void CommandPalette::refilter()
{
    const QString q = m_edit->text().trimmed();
    struct Hit { int idx; int score; };
    QList<Hit> hits;
    for (int i = 0; i < m_items.size(); ++i) {
        int sc = 0;
        if (q.isEmpty() || fuzzyMatch(q, m_items[i].title, &sc))
            hits.push_back({i, sc});
    }
    if (!q.isEmpty())
        std::stable_sort(hits.begin(), hits.end(), [](const Hit &a, const Hit &b) { return a.score > b.score; });
    m_list->clear();
    for (const Hit &h : std::as_const(hits)) {
        const PaletteItem &it = m_items[h.idx];
        auto *item = new QListWidgetItem(it.detail.isEmpty() ? it.title : it.title + QStringLiteral("    ") + it.detail, m_list);
        item->setData(Qt::UserRole, h.idx);
        if (!it.enabled)
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

int CommandPalette::visibleCount() const { return m_list->count(); }

void CommandPalette::setQuery(const QString &q) { m_edit->setText(q); }

void CommandPalette::runCurrent()
{
    auto *item = m_list->currentItem();
    if (!item || !(item->flags() & Qt::ItemIsEnabled))
        return;
    const PaletteItem it = m_items.value(item->data(Qt::UserRole).toInt());
    hide();
    if (it.run)
        it.run();
}

bool CommandPalette::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_edit && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Down:
            m_list->setCurrentRow(qMin(m_list->currentRow() + 1, m_list->count() - 1));
            return true;
        case Qt::Key_Up:
            m_list->setCurrentRow(qMax(m_list->currentRow() - 1, 0));
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            runCurrent();
            return true;
        case Qt::Key_Escape:
            hide();
            return true;
        default:
            break;
        }
    }
    return QDialog::eventFilter(obj, event);
}

} // namespace paw
