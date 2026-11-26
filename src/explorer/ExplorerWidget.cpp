#include "ExplorerWidget.h"

#include "explorer/FileOperations.h"
#include "explorer/HiddenFilterProxy.h"
#include "settings/Settings.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

namespace paw {

ExplorerWidget::ExplorerWidget(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Header: project name + actions
    auto *header = new QWidget(this);
    auto *hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 6, 6, 6);
    m_title = new QLabel(header);
    m_title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
    hl->addWidget(m_title, 1);
    const struct { const char *icon; const char *tip; } buttons[] = {
        {"newfile", "New File"}, {"newfolder", "New Folder"}, {"refresh", "Refresh"}, {"collapse", "Collapse All"}};
    for (const auto &b : buttons) {
        auto *tb = new QToolButton(header);
        tb->setIcon(icons::chrome(QLatin1String(b.icon), theme::textMuted, theme::text));
        tb->setToolTip(tr(b.tip));
        tb->setAutoRaise(true);
        tb->setProperty("action", QLatin1String(b.icon));
        hl->addWidget(tb);
        m_toolButtons.push_back(tb);
        connect(tb, &QToolButton::clicked, this, [this, tb] {
            const QString a = tb->property("action").toString();
            if (a == QLatin1String("newfile"))
                promptNewFile(m_root);
            else if (a == QLatin1String("newfolder"))
                promptNewFolder(m_root);
            else if (a == QLatin1String("refresh"))
                refresh();
            else
                m_view->collapseAll();
        });
    }
    root->addWidget(header);

    m_stack = new QStackedWidget(this);
    root->addWidget(m_stack, 1);

    // Empty page
    auto *empty = new QWidget(m_stack);
    auto *el = new QVBoxLayout(empty);
    el->setContentsMargins(16, 16, 16, 16);
    auto *msg = new QLabel(tr("You have not opened a folder yet."), empty);
    msg->setObjectName(QStringLiteral("muted"));
    msg->setWordWrap(true);
    auto *openBtn = new QPushButton(tr("Open Folder"), empty);
    openBtn->setObjectName(QStringLiteral("primary"));
    connect(openBtn, &QPushButton::clicked, this, &ExplorerWidget::openFolderRequested);
    el->addWidget(msg);
    el->addWidget(openBtn);
    el->addStretch(1);
    m_stack->addWidget(empty);

    // Tree page
    m_model = new QFileSystemModel(this);
    m_model->setReadOnly(false);
    m_proxy = new HiddenFilterProxy(this);
    m_proxy->setSourceModel(m_model);
    m_view = new QTreeView(m_stack);
    m_view->setModel(m_proxy);
    m_view->setHeaderHidden(true);
    m_view->setAnimated(true);
    m_view->setIndentation(16);
    m_view->setUniformRowHeights(true);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(0, Qt::AscendingOrder);
    for (int c = 1; c < m_model->columnCount(); ++c)
        m_view->hideColumn(c);
    m_stack->addWidget(m_view);

    connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &idx) {
        const QModelIndex src = m_proxy->mapToSource(idx);
        if (!m_model->isDir(src))
            emit openFileRequested(m_model->filePath(src));
    });
    connect(m_view, &QTreeView::customContextMenuRequested, this, &ExplorerWidget::showContextMenu);

    auto addShortcut = [this](const QKeySequence &ks, auto fn) {
        auto *sc = new QShortcut(ks, m_view);
        sc->setContext(Qt::WidgetShortcut);
        connect(sc, &QShortcut::activated, this, fn);
    };
    addShortcut(QKeySequence(Qt::Key_F2), [this] { promptRename(pathAt(m_view->currentIndex())); });
    addShortcut(QKeySequence(Qt::Key_Delete), [this] { deleteEntry(pathAt(m_view->currentIndex()), true); });
    addShortcut(QKeySequence::Copy, [this] {
        QStringList paths;
        for (const QModelIndex &i : m_view->selectionModel()->selectedRows())
            paths << m_model->filePath(m_proxy->mapToSource(i));
        copyEntries(paths);
    });
    addShortcut(QKeySequence::Paste, [this] { pasteInto(targetDirFor(pathAt(m_view->currentIndex()))); });

    setRootPath(QString());
    applySettings();
}

void ExplorerWidget::applySettings()
{
    // The source model always exposes hidden entries; whether they are
    // visible is decided by HiddenFilterProxy (see its comment for why the
    // QDir::Hidden filter alone is not enough on Windows).
    m_model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System | QDir::Hidden);
    m_proxy->setShowHidden(Settings::instance().showHiddenFiles());
}

void ExplorerWidget::setRootPath(const QString &path)
{
    m_root = path.isEmpty() ? QString() : QDir::cleanPath(path);
    const bool has = !m_root.isEmpty();
    for (QToolButton *b : m_toolButtons)
        b->setEnabled(has);
    if (!has) {
        m_title->setText(tr("EXPLORER"));
        m_stack->setCurrentIndex(0);
        return;
    }
    m_title->setText(QFileInfo(m_root).fileName().toUpper());
    m_title->setToolTip(QDir::toNativeSeparators(m_root));
    applySettings();
    const QModelIndex rootIdx = m_model->setRootPath(m_root);
    m_view->setRootIndex(m_proxy->mapFromSource(rootIdx));
    m_stack->setCurrentIndex(1);
}

void ExplorerWidget::refresh()
{
    if (m_root.isEmpty())
        return;
    const QString r = m_root;
    m_model->setRootPath(QString());
    setRootPath(r);
}

QString ExplorerWidget::pathAt(const QModelIndex &idx) const
{
    return idx.isValid() ? m_model->filePath(m_proxy->mapToSource(idx)) : QString();
}

QStringList ExplorerWidget::childNames(const QString &dir) const
{
    const QModelIndex src = m_model->index(QDir::cleanPath(dir));
    if (!src.isValid())
        return {};
    const QModelIndex parent = m_proxy->mapFromSource(src);
    QStringList names;
    for (int i = 0; i < m_proxy->rowCount(parent); ++i)
        names << m_proxy->index(i, 0, parent).data(QFileSystemModel::FileNameRole).toString();
    return names;
}

void ExplorerWidget::expandPath(const QString &path)
{
    const QModelIndex src = m_model->index(QDir::cleanPath(path));
    if (src.isValid())
        m_view->expand(m_proxy->mapFromSource(src));
}

QString ExplorerWidget::targetDirFor(const QString &path) const
{
    if (path.isEmpty())
        return m_root;
    return QFileInfo(path).isDir() ? path : QFileInfo(path).absolutePath();
}

void ExplorerWidget::error(const QString &title, const QString &msg)
{
    QMessageBox::warning(this, title, msg);
}

// ---- operations -----------------------------------------------------------

bool ExplorerWidget::createFileIn(const QString &dir, const QString &name)
{
    QString err;
    if (!fileops::validateName(name, &err)) {
        error(tr("New File"), err);
        return false;
    }
    const QString path = QDir(dir).filePath(name);
    if (!fileops::createFile(path, &err)) {
        error(tr("New File"), err);
        return false;
    }
    emit logMessage(tr("Created file %1").arg(QDir::toNativeSeparators(path)));
    emit openFileRequested(path);
    return true;
}

bool ExplorerWidget::createFolderIn(const QString &dir, const QString &name)
{
    QString err;
    if (!fileops::validateName(name, &err)) {
        error(tr("New Folder"), err);
        return false;
    }
    const QString path = QDir(dir).filePath(name);
    if (!fileops::createFolder(path, &err)) {
        error(tr("New Folder"), err);
        return false;
    }
    emit logMessage(tr("Created folder %1").arg(QDir::toNativeSeparators(path)));
    return true;
}

bool ExplorerWidget::renameEntry(const QString &path, const QString &newName)
{
    QString err;
    if (!fileops::renamePath(path, newName, &err)) {
        error(tr("Rename"), err);
        return false;
    }
    const QString target = QFileInfo(path).dir().filePath(newName);
    emit pathRenamed(path, target);
    emit logMessage(tr("Renamed %1 to %2").arg(QFileInfo(path).fileName(), newName));
    return true;
}

bool ExplorerWidget::deleteEntry(const QString &path, bool askConfirmation)
{
    if (path.isEmpty() || path == m_root)
        return false;
    const QFileInfo fi(path);
    if (askConfirmation && Settings::instance().confirmDelete()) {
        const QString what = fi.isDir() ? tr("the folder '%1' and everything inside it").arg(fi.fileName())
                                        : tr("the file '%1'").arg(fi.fileName());
        const auto r = QMessageBox::warning(this, tr("Delete"),
                                            tr("Are you sure you want to permanently delete %1?\n\nThis cannot be undone.").arg(what),
                                            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (r != QMessageBox::Yes)
            return false;
    }
    QString err;
    if (!fileops::removePath(path, &err)) {
        error(tr("Delete"), err);
        return false;
    }
    emit pathDeleted(path);
    emit logMessage(tr("Deleted %1").arg(QDir::toNativeSeparators(path)));
    return true;
}

bool ExplorerWidget::duplicateEntry(const QString &path)
{
    const QFileInfo fi(path);
    const QString dst = fi.dir().filePath(fileops::uniqueCopyName(fi.absolutePath(), fi.fileName()));
    QString err;
    if (!fileops::copyRecursively(path, dst, &err)) {
        error(tr("Duplicate"), err);
        return false;
    }
    return true;
}

void ExplorerWidget::copyEntries(const QStringList &paths)
{
    if (paths.isEmpty())
        return;
    m_clipboard = paths;
    auto *mime = new QMimeData;
    QList<QUrl> urls;
    for (const QString &p : paths)
        urls << QUrl::fromLocalFile(p);
    mime->setUrls(urls);
    QApplication::clipboard()->setMimeData(mime);
}

bool ExplorerWidget::pasteInto(const QString &dir)
{
    QStringList sources = m_clipboard;
    if (const QMimeData *md = QApplication::clipboard()->mimeData(); md && md->hasUrls()) {
        sources.clear();
        for (const QUrl &u : md->urls())
            if (u.isLocalFile())
                sources << u.toLocalFile();
    }
    if (sources.isEmpty() || dir.isEmpty())
        return false;
    bool ok = true;
    for (const QString &src : std::as_const(sources)) {
        const QFileInfo si(src);
        QString name = si.fileName();
        if (QFileInfo::exists(QDir(dir).filePath(name)))
            name = fileops::uniqueCopyName(dir, name);
        QString err;
        if (!fileops::copyRecursively(src, QDir(dir).filePath(name), &err)) {
            error(tr("Paste"), err);
            ok = false;
        }
    }
    return ok;
}

// ---- dialogs & menu -------------------------------------------------------

void ExplorerWidget::promptNewFile(const QString &dir)
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New File"), tr("File name:"), QLineEdit::Normal, QString(), &ok);
    if (ok)
        createFileIn(dir, name.trimmed());
}

void ExplorerWidget::promptNewFolder(const QString &dir)
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"), QLineEdit::Normal, QString(), &ok);
    if (ok)
        createFolderIn(dir, name.trimmed());
}

void ExplorerWidget::promptRename(const QString &path)
{
    if (path.isEmpty() || path == m_root)
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Rename"), tr("New name:"), QLineEdit::Normal,
                                               QFileInfo(path).fileName(), &ok);
    if (ok && name != QFileInfo(path).fileName())
        renameEntry(path, name.trimmed());
}

void ExplorerWidget::revealInFileManager(const QString &path)
{
#ifdef Q_OS_WIN
    QProcess p;
    p.setProgram(QStringLiteral("explorer.exe"));
    p.setNativeArguments(QStringLiteral("/select,\"%1\"").arg(QDir::toNativeSeparators(path)));
    p.startDetached();
#else
    const QFileInfo fi(path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? path : fi.absolutePath()));
#endif
}

void ExplorerWidget::showContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_view->indexAt(pos);
    const QString path = pathAt(idx);
    const QString dir = targetDirFor(path);
    QMenu menu(this);
    menu.addAction(tr("New File…"), this, [this, dir] { promptNewFile(dir); });
    menu.addAction(tr("New Folder…"), this, [this, dir] { promptNewFolder(dir); });
    if (!path.isEmpty()) {
        menu.addSeparator();
        if (!QFileInfo(path).isDir())
            menu.addAction(tr("Open"), this, [this, path] { emit openFileRequested(path); });
        QStringList selected;
        for (const QModelIndex &i : m_view->selectionModel()->selectedRows())
            selected << m_model->filePath(m_proxy->mapToSource(i));
        if (selected.isEmpty())
            selected << path;
        menu.addAction(tr("Copy"), this, [this, selected] { copyEntries(selected); });
        menu.addAction(tr("Paste"), this, [this, dir] { pasteInto(dir); });
        menu.addAction(tr("Duplicate"), this, [this, path] { duplicateEntry(path); });
        menu.addSeparator();
        menu.addAction(tr("Rename…"), this, [this, path] { promptRename(path); });
        menu.addAction(tr("Delete"), this, [this, path] { deleteEntry(path, true); });
    } else {
        menu.addAction(tr("Paste"), this, [this, dir] { pasteInto(dir); });
    }
    menu.addSeparator();
    menu.addAction(tr("Reveal in File Manager"), this, [this, path] { revealInFileManager(path.isEmpty() ? m_root : path); });
    menu.addAction(tr("Refresh"), this, [this] { refresh(); });
    menu.exec(m_view->viewport()->mapToGlobal(pos));
}

} // namespace paw
