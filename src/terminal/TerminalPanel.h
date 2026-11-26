#pragma once
#include <QProcess>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QToolButton;

namespace paw {

// Integrated terminal (bottom panel tab). Runs PowerShell / CMD on Windows
// and $SHELL or /bin/bash elsewhere. Line-based input; output is appended.
class TerminalPanel : public QWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget *parent = nullptr);
    ~TerminalPanel() override;

    bool isRunning() const { return m_shell && m_shell->state() == QProcess::Running; }
    void setWorkingDirectory(const QString &path); // restarts the shell when the project changes
    void stop();

public slots:
    void start();
    void focusInput();

protected:
    void showEvent(QShowEvent *e) override;

signals:
    void logMessage(const QString &line);

private:
    void runCommand();
    void appendOut(const QString &text);
    QString pickShell() const;
    QProcessEnvironment shellEnvironment() const;

    QProcess *m_shell = nullptr;
    QPlainTextEdit *m_out = nullptr;
    QLineEdit *m_in = nullptr;
    QLabel *m_title = nullptr;
    QPushButton *m_kill = nullptr;
    QString m_cwd;
    QString m_lastEcho; // command echo to avoid duplicating the shell's own echo
};

} // namespace paw
