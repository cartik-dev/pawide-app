#include "GitPanel.h"

#include "ui/Theme.h"

#include <QDir>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace paw {

namespace {
QString gitBinary()
{
    return QStringLiteral("git");
}
} // namespace

GitPanel::GitPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("SOURCE CONTROL"), this);
    title->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; letter-spacing: 1px; }"));
    layout->addWidget(title);

    m_branch = new QLabel(this);
    m_branch->setStyleSheet(QStringLiteral("QLabel { color: %1; font-weight: 600; }").arg(theme::accentLight.name()));
    layout->addWidget(m_branch);

    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("muted"));
    m_hint->setWordWrap(true);
    layout->addWidget(m_hint);

    m_files = new QTreeWidget(this);
    m_files->setHeaderHidden(true);
    m_files->setUniformRowHeights(true);
    m_files->setColumnCount(2);
    m_files->header()->setStretchLastSection(true);
    m_files->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_files->header()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_files->header()->resizeSection(1, 34);
    connect(m_files, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *it, int) {
        if (it)
            emit fileActivated(it->data(0, Qt::UserRole).toString());
    });
    layout->addWidget(m_files, 1);

    auto *tools = new QHBoxLayout;
    m_refreshBtn = new QPushButton(tr("Refresh"), this);
    m_stageBtn = new QPushButton(tr("Stage All"), this);
    m_unstageBtn = new QPushButton(tr("Unstage All"), this);
    for (QPushButton *b : {m_refreshBtn, m_stageBtn, m_unstageBtn})
        tools->addWidget(b);
    layout->addLayout(tools);

    m_message = new QLineEdit(this);
    m_message->setPlaceholderText(tr("Commit message (Commit = Stage All + commit)"));
    layout->addWidget(m_message);

    m_commitBtn = new QPushButton(tr("Commit"), this);
    m_commitBtn->setObjectName(QStringLiteral("primary"));
    connect(m_commitBtn, &QPushButton::clicked, this, &GitPanel::commit);
    connect(m_message, &QLineEdit::returnPressed, this, &GitPanel::commit);
    layout->addWidget(m_commitBtn);

    connect(m_refreshBtn, &QPushButton::clicked, this, &GitPanel::refresh);
    connect(m_stageBtn, &QPushButton::clicked, this, &GitPanel::stageAll);
    connect(m_unstageBtn, &QPushButton::clicked, this, &GitPanel::unstageAll);

    setRepository(QString());
}

void GitPanel::setRepository(const QString &path)
{
    m_repo.clear();
    if (!path.isEmpty()) {
        // The project folder itself or any of its parents may be a repository.
        QDir dir(path);
        for (;;) {
            if (QFileInfo::exists(dir.filePath(QStringLiteral(".git")))) {
                m_repo = dir.absolutePath();
                break;
            }
            if (!dir.cdUp())
                break;
        }
    }
    const bool repo = !m_repo.isEmpty();
    m_files->clear();
    m_message->clear();
    for (QPushButton *b : {m_refreshBtn, m_stageBtn, m_unstageBtn, m_commitBtn})
        b->setEnabled(repo);
    m_message->setEnabled(repo);
    if (repo) {
        m_hint->setText(tr("Repository: %1").arg(QDir::toNativeSeparators(m_repo)));
        refresh();
    } else {
        m_branch->setText(QString());
        m_hint->setText(tr("The current folder is not a Git repository.\nInitialise one with 'git init' to use Source Control."));
    }
    emit repositoryChanged(repo);
}

void GitPanel::runGit(const QStringList &args,
                      const std::function<void(const QString &, const QString &, int)> &onDone)
{
    auto *p = new QProcess(this);
    p->setWorkingDirectory(m_repo);
    p->setProcessChannelMode(QProcess::SeparateChannels);
    connect(p, &QProcess::finished, this, [this, p, onDone](int code, QProcess::ExitStatus) {
        const QString out = QString::fromLocal8Bit(p->readAllStandardOutput());
        const QString err = QString::fromLocal8Bit(p->readAllStandardError());
        p->deleteLater();
        if (code != 0 && !err.trimmed().isEmpty())
            emit logMessage(tr("git: %1").arg(err.trimmed()));
        if (onDone)
            onDone(out, err, code);
    });
    p->start(gitBinary(), args);
}

void GitPanel::parseStatus(const QString &out)
{
    m_files->clear();
    QString branch;
    const QStringList lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1String("## "))) {
            branch = line.mid(3).section(QLatin1Char('.'), 0, 0);
            continue;
        }
        if (line.length() < 4)
            continue;
        const QChar x = line.at(0); // staged
        const QChar y = line.at(1); // unstaged
        QString status;
        if (x == QLatin1Char('?'))
            status = tr("U");
        else if (x != QLatin1Char(' ') && x != QLatin1Char('?'))
            status = tr("S");
        else if (y != QLatin1Char(' '))
            status = tr("M");
        const QString file = line.mid(3).trimmed();
        auto *it = new QTreeWidgetItem(m_files);
        it->setText(0, file);
        it->setText(1, status);
        it->setData(0, Qt::UserRole, m_repo + QLatin1Char('/') + file);
        it->setToolTip(0, tr("Double-click to open; use Stage All to prepare a commit"));
        it->setForeground(1, status == tr("U") ? QColor(0xf0, 0xa3, 0x5e)
                            : status == tr("S") ? QColor(0x7e, 0xe7, 0x87)
                                                : QColor(0x79, 0xc0, 0xff));
    }
    m_branch->setText(branch.isEmpty() ? QStringLiteral("git") : QStringLiteral("\u2387  %1").arg(branch));
}

void GitPanel::refresh()
{
    if (m_repo.isEmpty())
        return;
    runGit({QStringLiteral("status"), QStringLiteral("--porcelain=v1"), QStringLiteral("-b")},
           [this](const QString &out, const QString &, int) { parseStatus(out); });
}

void GitPanel::stageAll()
{
    if (m_repo.isEmpty())
        return;
    runGit({QStringLiteral("add"), QStringLiteral("-A")}, [this](const QString &, const QString &, int) { refresh(); });
}

void GitPanel::unstageAll()
{
    if (m_repo.isEmpty())
        return;
    runGit({QStringLiteral("reset"), QStringLiteral("-q")}, [this](const QString &, const QString &, int) { refresh(); });
}

void GitPanel::commit()
{
    if (m_repo.isEmpty())
        return;
    const QString msg = m_message->text().trimmed();
    if (msg.isEmpty()) {
        m_message->setFocus();
        return;
    }
    // Commit = stage everything, then commit.
    runGit({QStringLiteral("add"), QStringLiteral("-A")},
           [this, msg](const QString &, const QString &, int) {
               runGit({QStringLiteral("commit"), QStringLiteral("-m"), msg},
                      [this](const QString &, const QString &, int code) {
                          m_message->clear();
                          refresh();
                          emit logMessage(code == 0 ? tr("Commit created.") : tr("Commit failed (see the log)."));
                      });
           });
}

void GitPanel::cloneInto(const QString &url, const QString &destination, QObject *context,
                         const std::function<void(const QString &)> &log, const std::function<void(bool)> &done)
{
    auto *p = new QProcess(context);
    p->setWorkingDirectory(QFileInfo(destination).absolutePath());
    p->setProcessChannelMode(QProcess::MergedChannels);
    connect(p, &QProcess::readyRead, context, [p, log] {
        for (const QString &l : QString::fromLocal8Bit(p->readAll()).split(QLatin1Char('\r'), Qt::SkipEmptyParts))
            log(l.trimmed());
    });
    connect(p, &QProcess::finished, context, [p, log, done](int code, QProcess::ExitStatus) {
        log(code == 0 ? QObject::tr("Repository cloned successfully.")
                      : QObject::tr("git clone failed with exit code %1.").arg(code));
        p->deleteLater();
        if (done)
            done(code == 0);
    });
    log(QObject::tr("Cloning %1 …").arg(url));
    p->start(gitBinary(), {QStringLiteral("clone"), QStringLiteral("--progress"), url, destination});
}

} // namespace paw
