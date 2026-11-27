#pragma once
#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;
class QProcess;

namespace paw {

// "Run and Debug" side view: named workflows (run configurations) executed in
// the project folder. Output lands in the bottom Output tab. Workflows persist
// per project in the SQLite database.
class RunPanel : public QWidget {
    Q_OBJECT
public:
    enum class Kind { Run, Debug, Task };

    struct Workflow {
        QString name;
        QString command;
        Kind kind = Kind::Run;
    };

    explicit RunPanel(QWidget *parent = nullptr);

    void setProject(const QString &path); // empty = no project
    void stopAll();
    bool isRunning() const { return m_proc != nullptr; }
    void runWorkflow(int index); // used by tests and the screenshot tool

signals:
    void logMessage(const QString &line);
    void runStarted(const QString &workflow);
    void runFinished(bool success);
    void requestOutput(); // the main window shows the bottom Output tab

private:
    void loadWorkflows();
    void saveWorkflows() const;
    void rebuildList();
    void start(const Workflow &wf);
    void addWorkflow();
    void removeCurrent();
    void setBusy(bool busy);

    QString m_project;
    QList<Workflow> m_workflows;
    QListWidget *m_list = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_hint = nullptr;
    QPushButton *m_runBtn = nullptr;
    QPushButton *m_debugBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QPushButton *m_addBtn = nullptr;
    QPushButton *m_removeBtn = nullptr;
    QProcess *m_proc = nullptr;
};

} // namespace paw
