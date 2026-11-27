#include "ExtensionsPanel.h"

#include "core/AppContext.h"
#include "core/PluginManager.h"
#include "database/Database.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>

namespace paw {

static QList<ExtensionsPanel::PluginInfo> defaultCatalog()
{
    using P = ExtensionsPanel::PluginInfo;
    return {
        {QStringLiteral("prettier"), QStringLiteral("Prettier Formatter"), QStringLiteral("prettier.io"),
         QStringLiteral("1.4.2"), QStringLiteral("Format C++, Python and web files on save with one consistent style.")},
        {QStringLiteral("githistory"), QStringLiteral("Git History Viewer"), QStringLiteral("paw-labs"),
         QStringLiteral("0.9.0"), QStringLiteral("Browse the full commit history of any file straight from the editor.")},
        {QStringLiteral("vim"), QStringLiteral("Vim Keymap"), QStringLiteral("vim-alliance"),
         QStringLiteral("2.1.0"), QStringLiteral("Modal editing: normal, visual and insert modes with the classic bindings.")},
        {QStringLiteral("theme-studio"), QStringLiteral("Theme Studio"), QStringLiteral("paw-labs"),
         QStringLiteral("1.0.3"), QStringLiteral("Design and share custom colour themes with a live preview.")},
        {QStringLiteral("langpack-ru"), QStringLiteral("Language Pack — Русский"), QStringLiteral("paw-labs"),
         QStringLiteral("1.2.0"), QStringLiteral("Localises every menu, dialog and message of the IDE.")},
        {QStringLiteral("todo"), QStringLiteral("TODO Highlighter"), QStringLiteral("dev-tools"),
         QStringLiteral("1.1.1"), QStringLiteral("Collects TODO and FIXME comments into the Problems panel.")},
        {QStringLiteral("rainbow"), QStringLiteral("Rainbow Brackets"), QStringLiteral("dev-tools"),
         QStringLiteral("1.0.0"), QStringLiteral("Colours matching brackets to make nested code easy to scan.")},
        {QStringLiteral("editorconfig"), QStringLiteral("EditorConfig Support"), QStringLiteral("editorconfig.org"),
         QStringLiteral("0.8.2"), QStringLiteral("Applies .editorconfig indentation and charset rules per project.")},
        {QStringLiteral("md-preview"), QStringLiteral("Markdown Preview"), QStringLiteral("paw-labs"),
         QStringLiteral("1.3.0"), QStringLiteral("Live rendered preview of Markdown files in a split pane.")},
        {QStringLiteral("metrics"), QStringLiteral("Code Metrics"), QStringLiteral("dev-tools"),
         QStringLiteral("0.6.4"), QStringLiteral("Cognitive complexity and maintainability index for the open file.")},
    };
}

ExtensionsPanel::ExtensionsPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("PLUGINS"), this);
    title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
    layout->addWidget(title);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("muted"));
    layout->addWidget(m_summary);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search plugins…"));
    m_search->setClearButtonEnabled(true);
    connect(m_search, &QLineEdit::textChanged, this, [this] { rebuildList(m_search->text()); });
    layout->addWidget(m_search);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setUniformItemSizes(false);
    m_list->setSpacing(6);
    layout->addWidget(m_list, 1);

    auto *pluginsFolder = new QPushButton(tr("Open plugins folder"), this);
    pluginsFolder->setCursor(Qt::PointingHandCursor);
    pluginsFolder->setProperty("kind", QStringLiteral("quiet"));
    connect(pluginsFolder, &QPushButton::clicked, this, [] {
        PluginManager::instance().scan();
        QDesktopServices::openUrl(QUrl::fromLocalFile(PluginManager::instance().pluginsDir()));
    });
    layout->addWidget(pluginsFolder);

    auto *official = new QLabel(tr("Official plugins: <a href='https://github.com/cartik-dev'>github.com/cartik-dev</a>"), this);
    official->setObjectName(QStringLiteral("muted"));
    official->setTextFormat(Qt::RichText);
    official->setOpenExternalLinks(true);
    official->setWordWrap(true);
    layout->addWidget(official);

    const QStringList saved = appDatabase().value(QStringLiteral("extensions.installed")).toStringList();
    m_installed = saved;

    rebuildList();
}

QStringList ExtensionsPanel::installedIds() const { return m_installed; }

void ExtensionsPanel::refresh() { rebuildList(m_search ? m_search->text() : QString()); }

void ExtensionsPanel::rebuildList(const QString &filter)
{
    if (m_catalog.isEmpty())
        m_catalog = defaultCatalog();
    PluginManager::instance().scan();

    m_list->clear();
    int shown = 0;
    const QString needle = filter.trimmed();
    for (const PluginInfo &p : std::as_const(m_catalog)) {
        if (!needle.isEmpty() && !p.name.contains(needle, Qt::CaseInsensitive)
            && !p.description.contains(needle, Qt::CaseInsensitive) && !p.author.contains(needle, Qt::CaseInsensitive))
            continue;
        ++shown;
        // Installed = marked in the database, or found on disk by the runtime.
        const bool onDisk = std::any_of(PluginManager::instance().plugins().cbegin(),
                                        PluginManager::instance().plugins().cend(),
                                        [&](const paw::PluginInfo &d) { return d.id == p.id; });
        const bool installed = onDisk || m_installed.contains(p.id);
        auto *item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, p.id);
        item->setSizeHint(QSize(0, installed ? 78 : 92));

        auto *row = new QWidget(m_list);
        auto *rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(10, 8, 10, 8);
        rowLayout->setSpacing(4);

        auto *head = new QHBoxLayout;
        const QString status = installed ? QStringLiteral("<span style='color:%1'>· %2</span>")
                                               .arg(theme::accentLight.name(),
                                                    onDisk ? tr("active") : tr("installed"))
                                         : QString();
        auto *name = new QLabel(QStringLiteral("<b>%1</b> <span style='color:%2'>v%3 · %4</span> %5")
                                    .arg(p.name, theme::textMuted.name(), p.version, p.author, status), row);
        name->setTextFormat(Qt::RichText);
        head->addWidget(name);
        head->addStretch(1);
        auto *btn = new QPushButton(installed ? tr("Installed ✓") : tr("Install"), row);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("kind", installed ? QStringLiteral("quiet") : QStringLiteral("accent"));
        btn->setEnabled(installed ? false : true);
        connect(btn, &QPushButton::clicked, this, [this, id = p.id] { toggleInstall(id); });
        head->addWidget(btn);
        rowLayout->addLayout(head);

        auto *desc = new QLabel(p.description, row);
        desc->setObjectName(QStringLiteral("muted"));
        desc->setWordWrap(true);
        rowLayout->addWidget(desc);

        if (!installed) {
            // The plugin runtime ships in a later phase; installing today stores
            // the plugin locally and shows a short progress simulation.
            auto *progress = new QLabel(row);
            progress->setObjectName(QStringLiteral("muted"));
            progress->hide();
            rowLayout->addWidget(progress);
        }
        m_list->setItemWidget(item, row);
    }
    int installedCount = m_installed.size();
    for (const paw::PluginInfo &d : PluginManager::instance().plugins())
        if (!m_installed.contains(d.id))
            ++installedCount;
    m_summary->setText(tr("%1 plugins · %2 installed").arg(m_catalog.size()).arg(installedCount));
    if (shown == 0) {
        auto *empty = new QListWidgetItem(tr("No plugins match your search."), m_list);
        empty->setFlags(Qt::NoItemFlags);
    }
}

void ExtensionsPanel::toggleInstall(const QString &id)
{
    for (int i = 0; i < m_list->count(); ++i) {
        auto *item = m_list->item(i);
        if (item->data(Qt::UserRole).toString() != id)
            continue;
        auto *row = qobject_cast<QWidget *>(m_list->itemWidget(item));
        if (!row)
            return;
        auto *btn = row->findChild<QPushButton *>();
        if (!btn || btn->text() != tr("Install"))
            return;
        btn->setEnabled(false);
        btn->setText(tr("Installing…"));
        // Downloading a real plugin package is a network concern of a later phase;
        // today the catalog entry is stored locally.
        QTimer::singleShot(900, this, [this, id] {
            if (!m_installed.contains(id))
                m_installed.push_back(id);
            appDatabase().setValue(QStringLiteral("extensions.installed"), m_installed);
            rebuildList(m_search ? m_search->text() : QString());
            emit logMessage(QStringLiteral("Plugin installed: %1").arg(id));
        });
        return;
    }
}

} // namespace paw
