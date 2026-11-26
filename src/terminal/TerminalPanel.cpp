#include "TerminalPanel.h"

#include "ui/Theme.h"

#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>

namespace paw {

TerminalPanel::TerminalPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *bar = new QWidget(this);
    bar->setStyleSheet(QStringLiteral("QWidget { background: %1; border-bottom: 1px solid %2; }")
                           .arg(theme::bgActivity.name(), theme::border.name()));
    auto *bl = new QHBoxLayout(bar);
    bl->setContentsMargins(10, 4, 10, 4);
    bl->setSpacing(8);
    m_title = new QLabel(tr("Terminal"), bar);
    m_title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; }"));
    bl->addWidget(m_title);
    bl->addStretch(1);
    auto *restart = new QPushButton(tr("New Terminal"), bar);
    restart->setStyleSheet(QStringLiteral("QPushButton { padding: 3px 10px; font-size: 11px; }"));
    connect(restart, &QPushButton::clicked, this, &TerminalPanel::start);
    bl->addWidget(restart);
    m_kill = new QPushButton(tr("Kill Terminal"), bar);
    m_kill->setStyleSheet(QStringLiteral("QPushButton { padding: 3px 10px; font-size: 11px; }"));
    connect(m_kill, &QPushButton::clicked, this, [this] { stop(); appendOut(tr("Terminal stopped.\r\n")); });
    bl->addWidget(m_kill);
    layout->addWidget(bar);

    m_out = new QPlainTextEdit(this);
    m_out->setReadOnly(true);
    m_out->setMaximumBlockCount(10000);
    m_out->setFrameShape(QFrame::NoFrame);
    layout->addWidget(m_out, 1);

    auto *inRow = new QWidget(this);
    auto *rl = new QHBoxLayout(inRow);
    rl->setContentsMargins(8, 4, 8, 6);
    rl->setSpacing(6);
    auto *prompt = new QLabel(QStringLiteral(">"), inRow);
    prompt->setStyleSheet(QStringLiteral("QLabel { color: %1; font-family: 'Consolas', monospace; }").arg(theme::accentLight.name()));
    rl->addWidget(prompt);
    m_in = new QLineEdit(inRow);
    m_in->setFrame(false);
    m_in->setFont(QFont(QStringLiteral("Consolas")));
    m_in->setStyleSheet(QStringLiteral("QLineEdit { background: %1; border-top: 1px solid %2; padding: 4px 0; }")
                            .arg(theme::bgEditor.name(), theme::border.name()));
    connect(m_in, &QLineEdit::returnPressed, this, &TerminalPanel::runCommand);
    rl->addWidget(m_in, 1);
    layout->addWidget(inRow);
}

TerminalPanel::~TerminalPanel()
{
    stop();
}

QString TerminalPanel::pickShell() const
{
#ifdef Q_OS_WIN
    const QString ps = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    if (!ps.isEmpty())
        return ps;
    const QString cmd = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
    return cmd.isEmpty() ? QStringLiteral("cmd.exe") : cmd;
#else
    const QString env = qEnvironmentVariable("SHELL");
    if (!env.isEmpty())
        return env;
    const QString bash = QStandardPaths::findExecutable(QStringLiteral("bash"));
    return bash.isEmpty() ? QStringLiteral("/bin/sh") : bash;
#endif
}

QProcessEnvironment TerminalPanel::shellEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("TERM"), QStringLiteral("dumb"));
    env.insert(QStringLiteral("PAGER"), QStringLiteral("cat"));
#ifdef Q_OS_WIN
    env.insert(QStringLiteral("PROMPT"), QStringLiteral("$P$G"));
#endif
    return env;
}

void TerminalPanel::start()
{
    stop();
    const QString cmd = pickShell();
    m_shell = new QProcess(this);
    m_shell->setWorkingDirectory(m_cwd.isEmpty() ? QDir::homePath() : m_cwd);
    m_shell->setProcessChannelMode(QProcess::MergedChannels);
    m_shell->setProcessEnvironment(shellEnvironment());
#ifdef Q_OS_WIN
    QStringList args;
    if (cmd.contains(QLatin1String("powershell")))
        args = {QStringLiteral("-NoLogo")};
    m_shell->start(cmd, args);
#else
    m_shell->start(cmd, QStringList());
#endif
    connect(m_shell, &QProcess::readyRead, this, [this] {
        appendOut(QString::fromLocal8Bit(m_shell->readAll()));
    });
    connect(m_shell, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        appendOut(tr("[terminal] could not start the shell.\r\n"));
        emit logMessage(tr("Could not start the terminal shell."));
    });
    connect(m_shell, &QProcess::finished, this, [this](int code) {
        appendOut(tr("\r\n[terminal] exited with code %1.\r\n").arg(code));
        m_in->setEnabled(false);
    });
    m_in->setEnabled(true);
    if (m_title)
        m_title->setText(tr("Terminal — %1").arg(QDir::toNativeSeparators(m_shell->workingDirectory())));
}

void TerminalPanel::setWorkingDirectory(const QString &path)
{
    if (path == m_cwd)
        return;
    m_cwd = path;
    if (isRunning()) // restart the shell so the new project folder becomes the cwd
        start();
}

void TerminalPanel::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    if (!m_shell) // lazy start: the shell spawns on first use, not on app launch
        start();
    focusInput();
}

void TerminalPanel::stop()
{
    if (!m_shell)
        return;
    if (m_shell->state() != QProcess::NotRunning) {
#ifdef Q_OS_WIN
        m_shell->terminate();
        if (!m_shell->waitForFinished(300))
            m_shell->kill();
#else
        m_shell->terminate();
#endif
    }
    m_shell->deleteLater();
    m_shell = nullptr;
}

void TerminalPanel::focusInput()
{
    m_in->setFocus();
}

void TerminalPanel::runCommand()
{
    if (!m_shell || m_shell->state() != QProcess::Running) {
        appendOut(tr("[terminal] no shell is running — click 'New Terminal'.\r\n"));
        return;
    }
    const QString cmd = m_in->text();
    m_in->clear();
    appendOut(QStringLiteral("> %1").arg(cmd));
    m_shell->write((cmd + QLatin1Char('\n')).toLocal8Bit());
}

void TerminalPanel::appendOut(const QString &text)
{
    QString clean = text;
    clean.remove(QLatin1Char('\r'));
    if (clean.endsWith(QLatin1Char('\n')))
        clean.chop(1);
    m_out->appendPlainText(clean);
}

} // namespace paw
