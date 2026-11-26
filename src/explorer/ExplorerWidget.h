#pragma once
#include <QWidget>

class QFileSystemModel;
class QLabel;
class QModelIndex;
class QStackedWidget;
class QToolButton;
class QTreeView;

namespace paw {

// File explorer for the current project folder (shows hidden files by default).
class ExplorerWidget : public QWidget {
    Q_OBJECT
public:
    explicit ExplorerWidget(QWidget *parent = nullptr);

    void setRootPath(const QString &path); // empty path = no project open
    QString rootPath() const { return m_root; }
    void refresh();
    void applySettings();

    QTreeView *view() const { return m_view; }
    QFileSystemModel *model() const { return m_model; }

    // Operations (also reachable from the context menu); return false on failure.
    bool createFileIn(const QString &dir, const QString &name);
    bool createFolderIn(const QString &dir, const QString &name);
    bool renameEntry(const QString &path, const QString &newName);
    bool deleteEntry(const QString &path, bool askConfirmation);
    bool duplicateEntry(const QString &path);
    void copyEntries(const QStringList &paths);
    bool pasteInto(const QString &dir);

signals:
    void openFileRequested(const QString &path);
    void openFolderRequested();
    void pathRenamed(const QString &oldPath, const QString &newPath);
    void pathDeleted(const QString &path);
    void logMessage(const QString &text);

private:
    void showContextMenu(const QPoint &pos);
    QString pathAt(const QModelIndex &idx) const;
    QString targetDirFor(const QString &path) const;
    void promptNewFile(const QString &dir);
    void promptNewFolder(const QString &dir);
    void promptRename(const QString &path);
    void revealInFileManager(const QString &path);
    void error(const QString &title, const QString &msg);

    QString m_root;
    QFileSystemModel *m_model;
    QTreeView *m_view;
    QStackedWidget *m_stack;
    QLabel *m_title;
    QList<QToolButton *> m_toolButtons;
    QStringList m_clipboard;
};

} // namespace paw
