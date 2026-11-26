#include "MainWindow.h"

#include "core/AppContext.h"
#include "core/AppInfo.h"
#include "database/Database.h"
#include "editor/CodeEditor.h"
#include "editor/EditorArea.h"
#include "explorer/ExplorerWidget.h"
#include "settings/Settings.h"
#include "ui/ActivityBar.h"
#include "ui/CommandPalette.h"
#include "ui/Icons.h"
#include "ui/NewProjectDialog.h"
#include "ui/SettingsDialog.h"
#include "ui/Theme.h"
#include "ui/WelcomeWidget.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace paw {

static QList<MainWindow *> &windowList()
{
    static QList<MainWindow *> l;
    return l;
}

QList<MainWindow *> MainWindow::windows() { return windowList(); }

MainWindow *MainWindow::createWindow()
{
    auto *w = new MainWindow;
    w->setAttribute(Qt::WA_DeleteOnClose);
    return w;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    windowList().push_back(this);
    setWindowIcon(icons::appIcon());
    setMinimumSize(820, 520);
    resize(1320, 820);
    buildUi();
    buildMenus();
    restoreLayout();
    if (windowList().size() > 1) // cascade additional windows
        move(pos() + QPoint(28 * (windowList().size() - 1), 28 * (windowList().size() - 1)));
    connect(&Settings::instance(), &Settings::changed, this, [this](const QString &) { applySettings(); });
    applySettings();
    updateTitle();
    updateStatus();
}

MainWindow::~MainWindow() { windowList().removeAll(this); }

// ---------------------------------------------------------------- UI

void MainWindow::buildUi()
{
    auto *root = new QWidget(this);
    auto *rl = new QHBoxLayout(root);
    rl->setContentsMargins(0, 0, 0, 0);
    rl->setSpacing(0);

    // Activity bar
    m_activity = new ActivityBar(root);
    const auto ic = [](const char *n) { return icons::chrome(QLatin1String(n), theme::textMuted, Qt::white); };
    m_activity->addItem(QStringLiteral("explorer"), tr("Explorer (Ctrl+Shift+E)"), ic("explorer"));
    m_activity->addItem(QStringLiteral("search"), tr("Search (Ctrl+Shift+F)"), ic("search"));
    m_activity->addItem(QStringLiteral("scm"), tr("Source Control (Ctrl+Shift+G)"), ic("git"));
    m_activity->addItem(QStringLiteral("run"), tr("Run & Debug (Ctrl+Shift+D)"), ic("run"));
    m_activity->addItem(QStringLiteral("extensions"), tr("Extensions (Ctrl+Shift+X)"), ic("extensions"));
    m_activity->addBottomAction(QStringLiteral("settings"), tr("Settings"), ic("settings"));
    rl->addWidget(m_activity);

    // Side bar
    m_sideHost = new QWidget(root);
    m_sideHost->setStyleSheet(QStringLiteral("QWidget#sideHost { background: %1; }").arg(theme::bgSideBar.name()));
    m_sideHost->setObjectName(QStringLiteral("sideHost"));
    auto *sl = new QVBoxLayout(m_sideHost);
    sl->setContentsMargins(0, 0, 0, 0);
    m_side = new QStackedWidget(m_sideHost);
    sl->addWidget(m_side);
    m_explorer = new ExplorerWidget(m_side);
    m_side->addWidget(m_explorer); // index 0
    auto placeholder = [this](const QString &title, const QString &text) {
        auto *w = new QWidget(m_side);
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(14, 12, 14, 12);
        auto *t = new QLabel(title.toUpper(), w);
        t->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
        auto *m = new QLabel(text, w);
        m->setObjectName(QStringLiteral("muted"));
        m->setWordWrap(true);
        l->addWidget(t);
        l->addWidget(m);
        l->addStretch(1);
        m_side->addWidget(w);
    };
    placeholder(tr("Search"), tr("Project-wide search is not available yet. It is planned for the next development phase."));
    placeholder(tr("Source Control"), tr("Git integration is not available yet. It is planned for a later development phase."));
    placeholder(tr("Run and Debug"), tr("Running and debugging are not available yet."));
    placeholder(tr("Extensions"), tr("The extension system and marketplace are not available yet. They are planned for a later development phase."));

    // Editor + welcome
    m_central = new QStackedWidget(root);
    m_welcome = new WelcomeWidget(m_central);
    m_editors = new EditorArea(m_central);
    m_central->addWidget(m_welcome);
    m_central->addWidget(m_editors);

    // Bottom panel
    m_bottom = new QTabWidget(root);
    m_bottom->setDocumentMode(true);
    auto *problems = new QLabel(tr("No problems have been detected in the workspace."), m_bottom);
    problems->setObjectName(QStringLiteral("muted"));
    problems->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    problems->setMargin(12);
    m_bottom->addTab(problems, tr("Problems"));
    m_output = new QPlainTextEdit(m_bottom);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(5000);
    m_bottom->addTab(m_output, tr("Output"));
    m_bottom->hide();

    m_vsplit = new QSplitter(Qt::Vertical, root);
    m_vsplit->addWidget(m_central);
    m_vsplit->addWidget(m_bottom);
    m_vsplit->setStretchFactor(0, 1);
    m_vsplit->setStretchFactor(1, 0);
    m_vsplit->setChildrenCollapsible(false);

    m_hsplit = new QSplitter(Qt::Horizontal, root);
    m_hsplit->addWidget(m_sideHost);
    m_hsplit->addWidget(m_vsplit);
    m_hsplit->setStretchFactor(0, 0);
    m_hsplit->setStretchFactor(1, 1);
    m_hsplit->setSizes({270, 1050});
    m_hsplit->setChildrenCollapsible(false);
    m_sideHost->setMinimumWidth(190);
    rl->addWidget(m_hsplit, 1);
    setCentralWidget(root);

    // Status bar
    m_stLang = new QLabel(this);
    m_stPos = new QLabel(this);
    m_stEnc = new QLabel(this);
    m_stEol = new QLabel(this);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(m_stPos);
    statusBar()->addPermanentWidget(m_stEnc);
    statusBar()->addPermanentWidget(m_stEol);
    statusBar()->addPermanentWidget(m_stLang);

    m_palette = new CommandPalette(this);

    // Wiring
    connect(m_activity, &ActivityBar::viewSelected, this, &MainWindow::showView);
    connect(m_activity, &ActivityBar::activeViewClicked, this, [this](const QString &) { toggleSideBar(); });
    connect(m_activity, &ActivityBar::bottomActionTriggered, this, [this](const QString &id) {
        if (id == QLatin1String("settings"))
            SettingsDialog(this).exec();
    });
    m_activity->setCurrent(QStringLiteral("explorer"));

    connect(m_welcome, &WelcomeWidget::openFolderRequested, this, [this] { openFolderDialog(false); });
    connect(m_welcome, &WelcomeWidget::newProjectRequested, this, &MainWindow::newProjectDialog);
    connect(m_welcome, &WelcomeWidget::openRecentRequested, this, &MainWindow::openRecent);
    connect(m_welcome, &WelcomeWidget::documentationRequested, this,
            [] { QDesktopServices::openUrl(QUrl(repositoryUrl())); });

    connect(m_explorer, &ExplorerWidget::openFileRequested, this, &MainWindow::openFile);
    connect(m_explorer, &ExplorerWidget::openFolderRequested, this, [this] { openFolderDialog(false); });
    connect(m_explorer, &ExplorerWidget::pathRenamed, m_editors, &EditorArea::pathRenamed);
    connect(m_explorer, &ExplorerWidget::pathDeleted, m_editors, &EditorArea::closeEditorsUnder);
    connect(m_explorer, &ExplorerWidget::logMessage, this, &MainWindow::log);

    connect(m_editors, &EditorArea::currentEditorChanged, this, [this](CodeEditor *) {
        updateStatus();
        updateTitle();
    });
    connect(m_editors, &EditorArea::editorStateChanged, this, [this] {
        updateStatus();
        updateTitle();
    });
    connect(m_editors, &EditorArea::anyModifiedChanged, this, &MainWindow::updateTitle);
    connect(m_editors, &EditorArea::message, this, [this](const QString &t, int ms) { statusBar()->showMessage(t, ms); });
}

QAction *MainWindow::addAction(QMenu *menu, const QString &text, const QKeySequence &shortcut, auto &&slot)
{
    QAction *a = menu->addAction(text);
    if (!shortcut.isEmpty())
        a->setShortcut(shortcut);
    connect(a, &QAction::triggered, this, slot);
    a->setProperty("menuName", m_currentMenuName);
    m_actions.push_back(a);
    return a;
}

void MainWindow::buildMenus()
{
    auto current = [this]() { return m_editors->currentEditor(); };

    // File
    QMenu *file = menuBar()->addMenu(tr("&File"));
    m_currentMenuName = tr("File");
    addAction(file, tr("New File"), QKeySequence::New, [this] { m_central->setCurrentWidget(m_editors); m_editors->newUntitled(); });
    addAction(file, tr("New Window"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), [] { MainWindow::createWindow()->show(); });
    file->addSeparator();
    addAction(file, tr("New Project…"), QKeySequence(), [this] { newProjectDialog(); });
    addAction(file, tr("Open File…"), QKeySequence::Open, [this] { openFileDialog(); });
    addAction(file, tr("Open Folder…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), [this] { openFolderDialog(false); });
    addAction(file, tr("Open Folder in New Window…"), QKeySequence(), [this] { openFolderDialog(true); });
    m_recentMenu = file->addMenu(tr("Open Recent"));
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::populateRecentMenu);
    addAction(file, tr("Close Folder"), QKeySequence(), [this] { closeProject(); });
    file->addSeparator();
    addAction(file, tr("Save"), QKeySequence::Save, [this] { m_editors->saveCurrent(); });
    addAction(file, tr("Save As…"), QKeySequence::SaveAs, [this] { m_editors->saveCurrentAs(); });
    addAction(file, tr("Save All"), QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S), [this] { m_editors->saveAll(); });
    file->addSeparator();
    addAction(file, tr("Close Editor"), QKeySequence(Qt::CTRL | Qt::Key_W), [this] { m_editors->closeCurrent(); });
    addAction(file, tr("Exit"), QKeySequence(Qt::CTRL | Qt::Key_Q), [this] { close(); });

    // Edit
    QMenu *edit = menuBar()->addMenu(tr("&Edit"));
    m_currentMenuName = tr("Edit");
    addAction(edit, tr("Undo"), QKeySequence::Undo, [=] { if (auto *e = current()) e->undo(); });
    addAction(edit, tr("Redo"), QKeySequence::Redo, [=] { if (auto *e = current()) e->redo(); });
    edit->addSeparator();
    addAction(edit, tr("Cut"), QKeySequence::Cut, [=] { if (auto *e = current()) e->cut(); });
    addAction(edit, tr("Copy"), QKeySequence::Copy, [=] { if (auto *e = current()) e->copy(); });
    addAction(edit, tr("Paste"), QKeySequence::Paste, [=] { if (auto *e = current()) e->paste(); });

    // Selection
    QMenu *sel = menuBar()->addMenu(tr("&Selection"));
    m_currentMenuName = tr("Selection");
    addAction(sel, tr("Select All"), QKeySequence::SelectAll, [=] { if (auto *e = current()) e->selectAll(); });

    // View
    QMenu *view = menuBar()->addMenu(tr("&View"));
    m_currentMenuName = tr("View");
    addAction(view, tr("Command Palette…"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P), [this] { showCommandPalette(); });
    view->addSeparator();
    addAction(view, tr("Explorer"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E), [this] { showView(QStringLiteral("explorer")); });
    addAction(view, tr("Search"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F), [this] { showView(QStringLiteral("search")); });
    addAction(view, tr("Source Control"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G), [this] { showView(QStringLiteral("scm")); });
    addAction(view, tr("Run and Debug"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D), [this] { showView(QStringLiteral("run")); });
    addAction(view, tr("Extensions"), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_X), [this] { showView(QStringLiteral("extensions")); });
    view->addSeparator();
    addAction(view, tr("Toggle Side Bar"), QKeySequence(Qt::CTRL | Qt::Key_B), [this] { toggleSideBar(); });
    addAction(view, tr("Toggle Panel"), QKeySequence(Qt::CTRL | Qt::Key_J), [this] { togglePanel(); });
    view->addSeparator();
    m_wordWrapAction = addAction(view, tr("Word Wrap"), QKeySequence(Qt::ALT | Qt::Key_Z),
                                 [] { Settings::instance().setWordWrap(!Settings::instance().wordWrap()); });
    m_wordWrapAction->setCheckable(true);
    m_hiddenAction = addAction(view, tr("Show Hidden Files"), QKeySequence(),
                               [] { Settings::instance().setShowHiddenFiles(!Settings::instance().showHiddenFiles()); });
    m_hiddenAction->setCheckable(true);

    // Go
    QMenu *go = menuBar()->addMenu(tr("&Go"));
    m_currentMenuName = tr("Go");
    addAction(go, tr("Go to File…"), QKeySequence(Qt::CTRL | Qt::Key_P), [this] { showQuickOpen(); });
    addAction(go, tr("Go to Line…"), QKeySequence(Qt::CTRL | Qt::Key_G), [this] { goToLine(); });

    // Help
    QMenu *help = menuBar()->addMenu(tr("&Help"));
    m_currentMenuName = tr("Help");
    addAction(help, tr("Welcome"), QKeySequence(), [this] { m_central->setCurrentWidget(m_welcome); });
    addAction(help, tr("Documentation"), QKeySequence(), [] { QDesktopServices::openUrl(QUrl(repositoryUrl())); });
    addAction(help, tr("Settings"), QKeySequence(Qt::CTRL | Qt::Key_Comma), [this] { SettingsDialog(this).exec(); });
    help->addSeparator();
    addAction(help, tr("About Paw IDE"), QKeySequence(), [this] { showAbout(); });

    // Shortcuts of window-level actions must also work when the menu is not open.
    for (QAction *a : std::as_const(m_actions))
        QWidget::addAction(a);
}

// ---------------------------------------------------------------- projects

bool MainWindow::openProject(const QString &path)
{
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) {
        QMessageBox::warning(this, tr("Open Folder"), tr("The folder '%1' does not exist or is not accessible.").arg(QDir::toNativeSeparators(path)));
        return false;
    }
    const QString canonical = QDir::cleanPath(fi.absoluteFilePath());
    if (canonical == m_project) {
        m_central->setCurrentWidget(m_editors);
        return true;
    }
    for (MainWindow *w : windowList()) { // already open in another window: bring it to the front
        if (w != this && w->m_project == canonical) {
            w->raise();
            w->activateWindow();
            return true;
        }
    }
    if (!m_editors->maybeSaveAll())
        return false;
    m_editors->closeAllWithoutAsking();
    m_project = canonical;
    m_explorer->setRootPath(canonical);
    appDatabase().addRecentProject(canonical);
    m_central->setCurrentWidget(m_editors);
    showView(QStringLiteral("explorer"));
    log(tr("Opened folder %1").arg(QDir::toNativeSeparators(canonical)));
    updateTitle();
    return true;
}

bool MainWindow::closeProject()
{
    if (m_project.isEmpty())
        return true;
    if (!m_editors->maybeSaveAll())
        return false;
    m_editors->closeAllWithoutAsking();
    m_project.clear();
    m_explorer->setRootPath(QString());
    m_central->setCurrentWidget(m_welcome);
    updateTitle();
    return true;
}

void MainWindow::openFile(const QString &path)
{
    m_central->setCurrentWidget(m_editors);
    m_editors->openFile(path);
}

void MainWindow::openFolderDialog(bool newWindow)
{
    const QString last = appDatabase().value(QStringLiteral("open.lastDir")).toString();
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Folder"),
                                                          last.isEmpty() ? QDir::homePath() : last);
    if (dir.isEmpty())
        return;
    appDatabase().setValue(QStringLiteral("open.lastDir"), QFileInfo(dir).absolutePath());
    if (newWindow && !m_project.isEmpty()) {
        MainWindow *w = MainWindow::createWindow();
        w->show();
        w->openProject(dir);
    } else {
        openProject(dir);
    }
}

void MainWindow::newProjectDialog()
{
    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    if (openProject(dlg.projectPath()) && !dlg.mainFile().isEmpty())
        openFile(dlg.mainFile());
}

void MainWindow::openFileDialog()
{
    const QString start = m_project.isEmpty() ? QDir::homePath() : m_project;
    const QString path = QFileDialog::getOpenFileName(this, tr("Open File"), start);
    if (!path.isEmpty())
        openFile(path);
}

void MainWindow::openRecent(const QString &path)
{
    if (!QFileInfo(path).isDir()) {
        const auto r = QMessageBox::question(this, tr("Open Recent"),
                                             tr("The folder '%1' no longer exists.\n\nRemove it from the recent projects list?").arg(QDir::toNativeSeparators(path)));
        if (r == QMessageBox::Yes)
            appDatabase().removeRecentProject(path);
        return;
    }
    openProject(path);
}

void MainWindow::populateRecentMenu()
{
    m_recentMenu->clear();
    const auto projects = appDatabase().recentProjects();
    if (projects.isEmpty()) {
        m_recentMenu->addAction(tr("No Recent Projects"))->setEnabled(false);
        return;
    }
    for (const auto &p : projects) {
        const QString path = p.path;
        m_recentMenu->addAction(QStringLiteral("%1 — %2").arg(p.name, QDir::toNativeSeparators(p.path)), this,
                                [this, path] { openRecent(path); });
    }
    m_recentMenu->addSeparator();
    m_recentMenu->addAction(tr("Clear Recent Projects"), this, [] { appDatabase().clearRecentProjects(); });
}

// ---------------------------------------------------------------- view helpers

void MainWindow::showView(const QString &id)
{
    static const QStringList order = {QStringLiteral("explorer"), QStringLiteral("search"), QStringLiteral("scm"),
                                      QStringLiteral("run"), QStringLiteral("extensions")};
    const int idx = order.indexOf(id);
    if (idx < 0)
        return;
    m_side->setCurrentIndex(idx);
    m_activity->setCurrent(id);
    m_sideHost->show();
    if (m_hsplit->sizes().value(0) < 10)
        m_hsplit->setSizes({270, width() - 270});
}

void MainWindow::toggleSideBar() { m_sideHost->setVisible(!m_sideHost->isVisible()); }

void MainWindow::togglePanel()
{
    m_bottom->setVisible(!m_bottom->isVisible());
    if (m_bottom->isVisible())
        m_vsplit->setSizes({height() - 260, 220});
}

void MainWindow::log(const QString &line)
{
    m_output->appendPlainText(QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), line));
}

void MainWindow::updateTitle()
{
    QString t;
    if (CodeEditor *e = m_editors->currentEditor()) {
        t += e->displayName();
        if (e->document()->isModified())
            t += QStringLiteral(" \u25CF");
        t += QStringLiteral(" — ");
    }
    if (!m_project.isEmpty())
        t += QFileInfo(m_project).fileName() + QStringLiteral(" — ");
    setWindowTitle(t + appName());
}

void MainWindow::updateStatus()
{
    CodeEditor *e = m_editors->currentEditor();
    m_stLang->setText(e ? e->language().name : QString());
    m_stPos->setText(e ? tr("Ln %1, Col %2").arg(e->currentLine()).arg(e->currentColumn()) : QString());
    m_stEnc->setText(e ? e->encoding() : QString());
    m_stEol->setText(e ? FileUtils::eolName(e->lineEnding()) + (e->hasMixedLineEndings() ? QStringLiteral("*") : QString()) : QString());
    m_stEol->setToolTip(e && e->hasMixedLineEndings() ? tr("The file had mixed line endings. They are normalised on save.") : QString());
}

void MainWindow::applySettings()
{
    auto &s = Settings::instance();
    m_explorer->applySettings();
    if (!m_project.isEmpty())
        m_explorer->refresh();
    m_editors->applySettings();
    if (m_wordWrapAction)
        m_wordWrapAction->setChecked(s.wordWrap());
    if (m_hiddenAction)
        m_hiddenAction->setChecked(s.showHiddenFiles());
}

void MainWindow::showCommandPalette()
{
    QList<PaletteItem> items;
    for (QAction *a : std::as_const(m_actions)) {
        QString text = a->text();
        text.remove(QLatin1Char('&'));
        if (a->text().isEmpty())
            continue;
        const QString menu = a->property("menuName").toString();
        QAction *act = a;
        items.push_back({menu.isEmpty() ? text : menu + QStringLiteral(": ") + text, a->shortcut().toString(QKeySequence::NativeText),
                         [act] { act->trigger(); }, a->isEnabled()});
    }
    m_palette->showItems(items, tr("Type a command"));
}

void MainWindow::showQuickOpen()
{
    if (m_project.isEmpty()) {
        statusBar()->showMessage(tr("Open a folder first."), 3000);
        return;
    }
    static const QStringList skipDirs = {QStringLiteral(".git"), QStringLiteral("node_modules"), QStringLiteral("target"),
                                         QStringLiteral("build"), QStringLiteral("__pycache__")};
    QList<PaletteItem> items;
    const QDir root(m_project);
    QDirIterator it(m_project, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext() && count < 20000) {
        const QString full = it.next();
        const QString rel = root.relativeFilePath(full);
        bool skip = false;
        for (const QString &d : skipDirs)
            if (rel.startsWith(d + QLatin1Char('/')) || rel.contains(QLatin1Char('/') + d + QLatin1Char('/')))
                skip = true;
        if (skip)
            continue;
        ++count;
        items.push_back({rel, QString(), [this, full] { openFile(full); }, true});
    }
    m_palette->showItems(items, tr("Search files by name"));
}

void MainWindow::goToLine()
{
    CodeEditor *e = m_editors->currentEditor();
    if (!e)
        return;
    bool ok = false;
    const int line = QInputDialog::getInt(this, tr("Go to Line"), tr("Line number (1–%1):").arg(e->blockCount()), e->currentLine(),
                                          1, qMax(1, e->blockCount()), 1, &ok);
    if (ok)
        e->goToLine(line);
}

void MainWindow::showAbout()
{
    QMessageBox box(this);
    box.setWindowTitle(tr("About Paw IDE"));
    box.setIconPixmap(icons::brandPixmap(QStringLiteral(":/branding/paw-icon.svg"), 72, devicePixelRatioF()));
    box.setText(tr("<b>Paw IDE</b> %1<br>Code without limits.<br><br><a href=\"%2\">%2</a>").arg(appVersion(), repositoryUrl()));
    box.setTextFormat(Qt::RichText);
    box.exec();
}

// ---------------------------------------------------------------- persistence

void MainWindow::saveLayout()
{
    auto &db = appDatabase();
    db.setValue(QStringLiteral("window.geometry"), QString::fromLatin1(saveGeometry().toBase64()));
    db.setValue(QStringLiteral("window.sideWidth"), m_sideHost->isVisible() ? m_hsplit->sizes().value(0) : 0);
}

void MainWindow::restoreLayout()
{
    auto &db = appDatabase();
    const QByteArray geo = QByteArray::fromBase64(db.value(QStringLiteral("window.geometry")).toString().toLatin1());
    if (!geo.isEmpty() && windowList().size() == 1)
        restoreGeometry(geo);
    const int side = db.value(QStringLiteral("window.sideWidth"), 270).toInt();
    if (side >= 190)
        m_hsplit->setSizes({side, qMax(400, width() - side)});
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (!m_editors->maybeSaveAll()) { // the user cancelled: keep the window and all unsaved work
        e->ignore();
        return;
    }
    saveLayout();
    e->accept();
}

} // namespace paw
