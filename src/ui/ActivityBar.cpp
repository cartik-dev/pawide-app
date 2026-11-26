#include "ActivityBar.h"

#include <QToolButton>
#include <QVBoxLayout>

namespace paw {

ActivityBar::ActivityBar(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(48);
    setObjectName(QStringLiteral("activityBar"));
    setStyleSheet(QStringLiteral("QWidget#activityBar { background: #08080a; border-right: 1px solid #222229; }"));
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 4, 0, 4);
    root->setSpacing(0);
    m_top = new QVBoxLayout;
    m_top->setSpacing(0);
    m_bottom = new QVBoxLayout;
    m_bottom->setSpacing(0);
    root->addLayout(m_top);
    root->addStretch(1);
    root->addLayout(m_bottom);
}

QToolButton *ActivityBar::makeButton(const QString &id, const QString &title, const QIcon &icon, bool checkable)
{
    auto *b = new QToolButton(this);
    b->setObjectName(QStringLiteral("activity"));
    b->setIcon(icon);
    b->setIconSize(QSize(24, 24));
    b->setToolTip(title);
    b->setCheckable(checkable);
    b->setAutoRaise(true);
    b->setFixedSize(48, 48);
    b->setProperty("viewId", id);
    b->setAccessibleName(title);
    m_buttons.insert(id, b);
    return b;
}

void ActivityBar::addItem(const QString &id, const QString &title, const QIcon &icon)
{
    QToolButton *b = makeButton(id, title, icon, true);
    m_top->addWidget(b);
    connect(b, &QToolButton::clicked, this, [this, id] {
        if (m_current == id) {
            emit activeViewClicked(id);
            m_buttons.value(id)->setChecked(m_buttons.value(id)->isChecked());
            return;
        }
        setCurrent(id);
        emit viewSelected(id);
    });
}

void ActivityBar::addBottomAction(const QString &id, const QString &title, const QIcon &icon)
{
    QToolButton *b = makeButton(id, title, icon, false);
    m_bottom->addWidget(b);
    connect(b, &QToolButton::clicked, this, [this, id] { emit bottomActionTriggered(id); });
}

void ActivityBar::setCurrent(const QString &id)
{
    m_current = id;
    for (auto it = m_buttons.cbegin(); it != m_buttons.cend(); ++it) {
        if (it.value()->isCheckable())
            it.value()->setChecked(it.key() == id);
    }
}

} // namespace paw
