#pragma once
#include <QMainWindow>

class QAction;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QSplitter;
class QStackedWidget;
class QTabWidget;

namespace paw {

class ActivityBar;
class CodeEditor;
class CommandPalette;
class EditorArea;
class ExplorerWidget;
class GitPanel;
class SearchPanel;
class TerminalPanel;
class UpdateChecker;
class WelcomeWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    static MainWindow *createWindow();
    static QList<MainWindow *> windows();

    bool openProject(const QString &path);
    bool closeProject();
    void openFile(const QString &path);
    QString projectPath() const { return m_project; }

    EditorArea *editorArea() const { return m_editors; }
    ExplorerWidget *explorer() const { return m_explorer; }
    WelcomeWidget *welcome() const { return m_welcome; }
    CommandPalette *palette() const { return m_palette; }
    QList<QAction *> allActions() const { return m_actions; }
    void log(const QString &line);

protected:
    void closeEvent(QCloseEvent *e) override;

private:
    void buildUi();
    void buildMenus();
    QAction *addAction(QMenu *menu, const QString &text, const QKeySequence &shortcut, auto &&slot);
    void showView(const QString &id);
    void toggleSideBar();
    void togglePanel();
    void toggleTerminal();
    void updateTitle();
    void updateStatus();
    void applySettings();
    void openFolderDialog(bool newWindow);
    void newProjectDialog();
    void openFileDialog();
    void cloneRepository();
    void checkForUpdates();
    void openRecent(const QString &path);
    void populateRecentMenu();
    void showCommandPalette();
    void showQuickOpen();
    void goToLine();
    void showAbout();
    void saveLayout();
    void restoreLayout();

    QString m_project;
    ActivityBar *m_activity = nullptr;
    QStackedWidget *m_side = nullptr;
    QWidget *m_sideHost = nullptr;
    QStackedWidget *m_central = nullptr;
    QSplitter *m_hsplit = nullptr;
    QSplitter *m_vsplit = nullptr;
    QTabWidget *m_bottom = nullptr;
    QPlainTextEdit *m_output = nullptr;
    ExplorerWidget *m_explorer = nullptr;
    SearchPanel *m_search = nullptr;
    GitPanel *m_git = nullptr;
    TerminalPanel *m_terminal = nullptr;
    EditorArea *m_editors = nullptr;
    WelcomeWidget *m_welcome = nullptr;
    CommandPalette *m_palette = nullptr;
    UpdateChecker *m_updater = nullptr;
    QString m_pendingPath; // search-result navigation: open, then go to the line
    int m_pendingLine = 0;
    QLabel *m_stLang = nullptr, *m_stPos = nullptr, *m_stEnc = nullptr, *m_stEol = nullptr;
    QMenu *m_recentMenu = nullptr;
    QAction *m_wordWrapAction = nullptr;
    QAction *m_hiddenAction = nullptr;
    QList<QAction *> m_actions; // everything shown in the Command Palette
    QString m_currentMenuName;
};

} // namespace paw
