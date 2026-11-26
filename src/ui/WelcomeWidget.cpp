#include "WelcomeWidget.h"

#include "core/AppContext.h"
#include "core/AppInfo.h"
#include "database/Database.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace paw {

static QString relativeTime(const QDateTime &t)
{
    const qint64 secs = t.secsTo(QDateTime::currentDateTime());
    if (secs < 60)
        return QObject::tr("just now");
    if (secs < 3600)
        return QObject::tr("%1 min ago").arg(secs / 60);
    if (secs < 86400)
        return QObject::tr("%1 h ago").arg(secs / 3600);
    if (secs < 86400 * 30)
        return QObject::tr("%1 d ago").arg(secs / 86400);
    return t.toString(QStringLiteral("yyyy-MM-dd"));
}

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent)
{
    setStyleSheet(QStringLiteral("QWidget#welcome { background: %1; }").arg(theme::bgEditor.name()));
    setObjectName(QStringLiteral("welcome"));
    auto *outer = new QHBoxLayout(this);
    outer->addStretch(1);
    auto *col = new QVBoxLayout;
    col->setSpacing(10);
    col->setContentsMargins(0, 72, 0, 36);

    auto *head = new QHBoxLayout;
    head->setSpacing(18);
    auto *logo = new QLabel(this);
    logo->setPixmap(icons::brandPixmap(QStringLiteral(":/branding/paw-logo.svg"), 84, devicePixelRatioF()));
    logo->setFixedSize(84, 84);
    auto *titles = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("Paw <span style='color:#8b8b95;font-weight:300'>IDE</span>"), this);
    title->setTextFormat(Qt::RichText);
    title->setStyleSheet(QStringLiteral("QLabel { font-size: 34px; font-weight: 700; letter-spacing: 2px; }"));
    auto *sub = new QLabel(tr("Code without limits."), this);
    sub->setObjectName(QStringLiteral("muted"));
    auto *ver = new QLabel(tr("Version %1").arg(appVersion()), this);
    ver->setObjectName(QStringLiteral("muted"));
    titles->setSpacing(2);
    titles->addWidget(title);
    titles->addWidget(sub);
    titles->addWidget(ver);
    head->setAlignment(Qt::AlignTop);
    head->addWidget(logo);
    head->addLayout(titles);
    head->addStretch(1);
    col->addLayout(head);
    col->addSpacing(20);

    auto *startTitle = new QLabel(tr("Start"), this);
    startTitle->setStyleSheet(QStringLiteral("QLabel { font-size: 15px; font-weight: 600; }"));
    col->addWidget(startTitle);
    auto addLink = [&](const QString &text, auto signal) {
        auto *b = new QPushButton(text, this);
        b->setObjectName(QStringLiteral("link"));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, signal);
        col->addWidget(b);
    };
    addLink(tr("Open Folder…"), &WelcomeWidget::openFolderRequested);
    addLink(tr("New Project…"), &WelcomeWidget::newProjectRequested);
    addLink(tr("Documentation"), &WelcomeWidget::documentationRequested);
    col->addSpacing(18);

    auto *recentHead = new QHBoxLayout;
    m_recentTitle = new QLabel(tr("Recent"), this);
    m_recentTitle->setStyleSheet(QStringLiteral("QLabel { font-size: 15px; font-weight: 600; }"));
    m_clear = new QPushButton(tr("Clear"), this);
    m_clear->setObjectName(QStringLiteral("link"));
    m_clear->setCursor(Qt::PointingHandCursor);
    recentHead->addWidget(m_recentTitle);
    recentHead->addStretch(1);
    recentHead->addWidget(m_clear);
    col->addLayout(recentHead);

    m_empty = new QLabel(tr("No recent projects yet."), this);
    m_empty->setObjectName(QStringLiteral("muted"));
    col->addWidget(m_empty);
    m_list = new QListWidget(this);
    m_list->setStyleSheet(QStringLiteral("QListWidget { background: transparent; } QListWidget::item { padding: 0; }"));
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    col->addWidget(m_list, 1);
    col->addStretch(0);

    auto *colHost = new QWidget(this);
    colHost->setLayout(col);
    colHost->setFixedWidth(560);
    colHost->setStyleSheet(QStringLiteral("QWidget { background: transparent; }"));
    outer->addWidget(colHost);
    outer->addStretch(1);

    connect(m_clear, &QPushButton::clicked, this, [] { appDatabase().clearRecentProjects(); });
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *it) {
        emit openRecentRequested(it->data(Qt::UserRole).toString());
    });
    connect(&appDatabase(), &Database::recentProjectsChanged, this, &WelcomeWidget::reload);
    reload();
}

void WelcomeWidget::reload()
{
    m_list->clear();
    const auto projects = appDatabase().recentProjects();
    m_empty->setVisible(projects.isEmpty());
    m_list->setVisible(!projects.isEmpty());
    m_clear->setVisible(!projects.isEmpty());
    for (const auto &p : projects) {
        auto *item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, p.path);
        item->setSizeHint(QSize(0, 52));
        auto *row = new QWidget;
        row->setStyleSheet(QStringLiteral("QWidget { background: transparent; }"));
        auto *rl = new QHBoxLayout(row);
        rl->setContentsMargins(8, 4, 4, 4);
        auto *texts = new QVBoxLayout;
        texts->setSpacing(0);
        auto *name = new QLabel(p.name, row);
        name->setStyleSheet(QStringLiteral("QLabel { color: %1; font-weight: 600; }").arg(theme::accentLight.name()));
        auto *path = new QLabel(QDir::toNativeSeparators(p.path), row);
        path->setObjectName(QStringLiteral("muted"));
        path->setTextInteractionFlags(Qt::NoTextInteraction);
        texts->addWidget(name);
        texts->addWidget(path);
        auto *when = new QLabel(relativeTime(p.lastOpened), row);
        when->setObjectName(QStringLiteral("muted"));
        auto *remove = new QToolButton(row);
        remove->setIcon(icons::chrome(QStringLiteral("close"), theme::textMuted, theme::text));
        remove->setToolTip(tr("Remove from Recent (the folder is not deleted)"));
        const QString pathValue = p.path;
        connect(remove, &QToolButton::clicked, this, [pathValue] { appDatabase().removeRecentProject(pathValue); });
        rl->addLayout(texts, 1);
        rl->addWidget(when);
        rl->addWidget(remove);
        m_list->setItemWidget(item, row);
    }
}

} // namespace paw
