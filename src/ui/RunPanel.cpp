#include "RunPanel.h"

#include "core/AppContext.h"
#include "database/Database.h"
#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QVBoxLayout>

namespace paw {

static QString workflowsKey(const QString &project)
{
    if (project.isEmpty())
        return QStringLiteral("run.workflows.default");
    return QStringLiteral("run.workflows.%1").arg(qHash(project));
}

static QList<RunPanel::Workflow> defaultWorkflows(const QString &project)
{
    using Kind = RunPanel::Kind;
    if (QFileInfo::exists(QDir(project).filePath(QStringLiteral("CMakeLists.txt")))) {
        return {
            {QStringLiteral("Build & Run"), QStringLiteral("cmake --build build && ./build/app"), Kind::Run},
            {QStringLiteral("Run Tests"), QStringLiteral("ctest --test-dir build --output-on-failure"), Kind::Task},
            {QStringLiteral("Debug"), QStringLiteral("gdb build/app"), Kind::Debug},
        };
    }
    return {{QStringLiteral("Open Folder"), QStringLiteral("echo Open a folder to configure workflows"), Kind::Task}};
}

RunPanel::RunPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("RUN AND DEBUG"), this);
    title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
    layout->addWidget(title);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("muted"));
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_list, 1);

    auto *buttons = new QHBoxLayout;
    auto mkBtn = [this](const QString &icon, const QString &tip) {
        auto *b = new QPushButton(icons::chrome(icon, theme::textMuted, theme::text), QString(), this);
        b->setToolTip(tip);
        b->setCursor(Qt::PointingHandCursor);
        b->setFixedSize(34, 30);
        b->setIconSize(QSize(17, 17));
        return b;
    };
    m_runBtn = mkBtn(QStringLiteral("play"), tr("Run (Ctrl+F5)"));
    m_debugBtn = mkBtn(QStringLiteral("bug"), tr("Debug (F5)"));
    m_stopBtn = mkBtn(QStringLiteral("stop"), tr("Stop (Shift+F5)"));
    m_addBtn = mkBtn(QStringLiteral("plus"), tr("New Workflow…"));
    m_removeBtn = mkBtn(QStringLiteral("close"), tr("Delete Workflow"));
    buttons->addWidget(m_runBtn);
    buttons->addWidget(m_debugBtn);
    buttons->addWidget(m_stopBtn);
    buttons->addStretch(1);
    buttons->addWidget(m_addBtn);
    buttons->addWidget(m_removeBtn);
    layout->addLayout(buttons);

    m_hint = new QLabel(tr("A workflow is a named command that runs in the project folder. "
                           "The editor launches it as a child process and streams its output to the Output tab."),
                        this);
    m_hint->setObjectName(QStringLiteral("muted"));
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);

    connect(m_list, &QListWidget::itemDoubleClicked, this, [this] { m_runBtn->click(); });
    connect(m_runBtn, &QPushButton::clicked, this, [this] {
        const int idx = m_list->currentRow();
        if (idx >= 0 && idx < m_workflows.size() && !isRunning())
            start(m_workflows.at(idx));
    });
    connect(m_debugBtn, &QPushButton::clicked, this, [this] {
        const int idx = m_list->currentRow();
        if (idx < 0 || idx >= m_workflows.size() || isRunning())
            return;
        Workflow wf = m_workflows.at(idx);
        if (wf.kind != Kind::Debug) {
            wf.name = QStringLiteral("Debug: ") + wf.name;
            wf.kind = Kind::Debug;
        }
        start(wf);
    });
    connect(m_stopBtn, &QPushButton::clicked, this, [this] { stopAll(); });
    connect(m_addBtn, &QPushButton::clicked, this, [this] { addWorkflow(); });
    connect(m_removeBtn, &QPushButton::clicked, this, [this] { removeCurrent(); });
    setBusy(false);
}

void RunPanel::setProject(const QString &path)
{
    if (m_project == path)
        return;
    stopAll();
    m_project = path;
    loadWorkflows();
    rebuildList();
}

void RunPanel::loadWorkflows()
{
    const QJsonDocument doc = QJsonDocument::fromJson(appDatabase().value(workflowsKey(m_project)).toString().toUtf8());
    m_workflows.clear();
    const QJsonArray arr = doc.array();
    for (const auto &v : arr) {
        const QJsonObject o = v.toObject();
        Workflow wf;
        wf.name = o.value(QStringLiteral("name")).toString();
        wf.command = o.value(QStringLiteral("command")).toString();
        wf.kind = o.value(QStringLiteral("kind")).toInt() == 2 ? Kind::Task
                  : o.value(QStringLiteral("kind")).toInt() == 1 ? Kind::Debug
                                                                 : Kind::Run;
        if (!wf.name.isEmpty() && !wf.command.isEmpty())
            m_workflows.push_back(wf);
    }
    if (m_workflows.isEmpty())
        m_workflows = defaultWorkflows(m_project);
}

void RunPanel::saveWorkflows() const
{
    QJsonArray arr;
    for (const Workflow &wf : m_workflows) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), wf.name);
        o.insert(QStringLiteral("command"), wf.command);
        o.insert(QStringLiteral("kind"), static_cast<int>(wf.kind));
        arr.append(o);
    }
    appDatabase().setValue(workflowsKey(m_project), QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

void RunPanel::rebuildList()
{
    m_list->clear();
    for (const Workflow &wf : std::as_const(m_workflows)) {
        const QString kind = wf.kind == Kind::Debug ? QStringLiteral("DEBUG")
                            : wf.kind == Kind::Task ? QStringLiteral("TASK")
                                                              : QStringLiteral("RUN");
        auto *item = new QListWidgetItem(QStringLiteral("%1\n%2 · %3").arg(wf.name, kind, wf.command), m_list);
        item->setData(Qt::UserRole, static_cast<int>(wf.kind));
    }
    if (m_workflows.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No workflows yet. Add one with ＋."), m_list);
        item->setFlags(Qt::NoItemFlags);
    }
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void RunPanel::start(const Workflow &wf)
{
    if (m_project.isEmpty()) {
        QMessageBox::information(this, tr("Run and Debug"),
                                 tr("Open a folder first — workflows run inside the project directory."));
        return;
    }
    if (m_proc)
        return;

    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(m_project);
    m_proc->setProgram(QStringLiteral("/bin/sh"));
    m_proc->setArguments({QStringLiteral("-c"), wf.command});
#ifdef Q_OS_WIN
    m_proc->setProgram(QStringLiteral("cmd.exe"));
    m_proc->setArguments({QStringLiteral("/C"), wf.command});
#endif
    connect(m_proc, &QProcess::readyReadStandardOutput, this, [this] {
        emit logMessage(QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed());
    });
    connect(m_proc, &QProcess::readyReadStandardError, this, [this] {
        emit logMessage(QString::fromUtf8(m_proc->readAllStandardError()).trimmed());
    });
    connect(m_proc, &QProcess::finished, this, [this, wf](int code, QProcess::ExitStatus st) {
        emit logMessage(st == QProcess::CrashExit ? QStringLiteral("Process crashed.")
                                                  : QStringLiteral("Process exited with code %1").arg(code));
        setBusy(false);
        emit runFinished(code == 0 && st == QProcess::NormalExit);
        m_status->setText(st == QProcess::CrashExit
                              ? tr("Crashed while running “%1”.").arg(wf.name)
                              : code == 0 ? tr("“%1” finished successfully.").arg(wf.name)
                                          : tr("“%1” exited with code %2.").arg(wf.name).arg(code));
        m_proc->deleteLater();
        m_proc = nullptr;
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this, wf](QProcess::ProcessError) {
        emit logMessage(tr("Failed to start “%1”. Is the command available on this machine?").arg(wf.command));
        setBusy(false);
        emit runFinished(false);
        m_proc->deleteLater();
        m_proc = nullptr;
    });

    setBusy(true);
    m_status->setText(wf.kind == Kind::Debug ? tr("Debugging “%1”…").arg(wf.name)
                                                       : tr("Running “%1”…").arg(wf.name));
    emit runStarted(wf.name);
    emit requestOutput();
    m_proc->start();
}

void RunPanel::runWorkflow(int index)
{
    if (index >= 0 && index < m_workflows.size() && !isRunning())
        start(m_workflows.at(index));
}

void RunPanel::addWorkflow()
{
    if (m_project.isEmpty()) {
        QMessageBox::information(this, tr("Run and Debug"), tr("Open a folder first."));
        return;
    }
    const QString name = QInputDialog::getText(this, tr("New Workflow"), tr("Workflow name:"));
    if (name.trimmed().isEmpty())
        return;
    const QString command = QInputDialog::getText(this, tr("New Workflow"),
                                                  tr("Command to run in the project folder:"));
    if (command.trimmed().isEmpty())
        return;
    const QStringList kinds = {tr("Run"), tr("Debug"), tr("Task")};
    bool ok = false;
    const QString kind = QInputDialog::getItem(this, tr("New Workflow"), tr("Type:"), kinds, 0, false, &ok);
    if (!ok)
        return;
    Workflow wf;
    wf.name = name.trimmed();
    wf.command = command.trimmed();
    wf.kind = kind == tr("Debug") ? Kind::Debug : kind == tr("Task") ? Kind::Task : Kind::Run;
    m_workflows.push_back(wf);
    saveWorkflows();
    rebuildList();
    emit logMessage(tr("Workflow added: %1").arg(wf.name));
}

void RunPanel::removeCurrent()
{
    const int idx = m_list->currentRow();
    if (idx < 0 || idx >= m_workflows.size())
        return;
    m_workflows.removeAt(idx);
    saveWorkflows();
    rebuildList();
}

void RunPanel::stopAll()
{
    if (m_proc) {
        m_proc->kill();
        m_proc->waitForFinished(1000);
    }
}

void RunPanel::setBusy(bool busy)
{
    m_runBtn->setEnabled(!busy);
    m_debugBtn->setEnabled(!busy);
    m_stopBtn->setEnabled(busy);
    m_addBtn->setEnabled(!busy);
    m_removeBtn->setEnabled(!busy);
}

} // namespace paw
