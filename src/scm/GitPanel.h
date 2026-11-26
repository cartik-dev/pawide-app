#pragma once
#include <QString>
#include <QWidget>

#include <functional>

class QLabel;
class QLineEdit;
class QProcess;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace paw {

// Source Control side panel backed by the git CLI: status list, stage/unstage,
// commit and branch display. "Clone Repository" is a static helper.
class GitPanel : public QWidget {
    Q_OBJECT
public:
    explicit GitPanel(QWidget *parent = nullptr);

    void setRepository(const QString &path); // empty = no project / not a repo
    void refresh();
    bool isRepository() const { return !m_repo.isEmpty(); }

    static void cloneInto(const QString &url, const QString &destination, QObject *context,
                          const std::function<void(const QString &)> &log,
                          const std::function<void(bool)> &done);

signals:
    void logMessage(const QString &line);
    void fileActivated(const QString &path);
    void diffRequested(const QString &path);
    void repositoryChanged(bool isRepo);

private:
    void runGit(const QStringList &args,
                const std::function<void(const QString &out, const QString &err, int exitCode)> &onDone);
    void parseStatus(const QString &out);
    void stageAll();
    void unstageAll();
    void commit();

    QString m_repo;
    QLabel *m_branch = nullptr;
    QLabel *m_hint = nullptr;
    QLineEdit *m_message = nullptr;
    QTreeWidget *m_files = nullptr;
    QPushButton *m_commitBtn = nullptr;
    QPushButton *m_stageBtn = nullptr;
    QPushButton *m_unstageBtn = nullptr;
    QPushButton *m_refreshBtn = nullptr;
};
