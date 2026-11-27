#include "SandboxVerifier.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>

namespace paw {

QString SandboxVerifier::findCompiler()
{
    for (const QString &name : {QStringLiteral("g++"), QStringLiteral("clang++"), QStringLiteral("c++")}) {
        QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty())
            return path;
    }
    for (const QString &p : {QStringLiteral("/usr/bin/g++"), QStringLiteral("/usr/local/bin/g++"),
                             QStringLiteral("/mingw64/bin/g++"), QStringLiteral("C:/MinGW/bin/g++.exe"),
                             QStringLiteral("C:/msys64/mingw64/bin/g++.exe")}) {
        if (QFileInfo::exists(p))
            return p;
    }
    return {};
}

SandboxVerifier::SandboxVerifier(QObject *parent) : QObject(parent)
{
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(15000);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (m_proc)
            m_proc->kill();
    });
}

bool SandboxVerifier::verify(const QString &sourceText, const QString &fileName, const QString &languageId)
{
    if (m_proc) // one check at a time
        return false;
    if (languageId != QLatin1String("cpp") && languageId != QLatin1String("c"))
        return false;
    m_compiler = findCompiler();
    if (m_compiler.isEmpty())
        return false;

    // Isolated environment: a throw-away temporary directory. It exists only
    // while the check runs and is wiped afterwards — nothing is stored anywhere.
    m_sandbox = new QTemporaryDir(QDir::tempPath() + QStringLiteral("/pawide-sandbox-XXXXXX"));
    if (!m_sandbox->isValid()) {
        delete m_sandbox;
        m_sandbox = nullptr;
        return false;
    }
    const QString srcPath = QDir(m_sandbox->path()).filePath(fileName.isEmpty() ? QStringLiteral("main.cpp") : fileName);
    QFile f(srcPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    f.write(sourceText.toUtf8());
    f.close();

    QStringList args = {QStringLiteral("-x"), languageId == QLatin1String("c") ? QStringLiteral("c") : QStringLiteral("c++"),
                        QStringLiteral("-fsyntax-only"), QStringLiteral("-Wall"), QStringLiteral("-Wextra"), srcPath};

    emit progress(QStringLiteral("Compiling in an isolated environment…"));
    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(m_sandbox->path());
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) { collectResult(); });
    connect(m_proc, &QProcess::finished, this, [this](int, QProcess::ExitStatus) { collectResult(); });
    m_proc->start(m_compiler, args);
    m_timeout.start();
    return true;
}

void SandboxVerifier::collectResult()
{
    if (!m_proc)
        return;
    QProcess *p = m_proc;
    m_proc = nullptr;
    m_timeout.stop();
    const QString out = QString::fromUtf8(p->readAllStandardError());
    if (out.isEmpty())
        qWarning("sandbox: compiler produced no diagnostics on stderr");
    p->deleteLater();
    delete m_sandbox; // wipe the isolated environment — nothing was saved
    m_sandbox = nullptr;

    QList<CompilerDiag> diags;
    // Line format: <sandbox path>:line:col: severity: message
    static const QRegularExpression rx(QStringLiteral(":(\\d+):(\\d+):\\s+(warning|error|note):\\s+(.*)$"));
    for (const QString &lineView : out.split(QLatin1Char('\n'))) {
        const QString line = lineView.trimmed();
        if (line.isEmpty() || line.contains(QStringLiteral("In file included")))
            continue;
        const auto m = rx.match(line);
        if (!m.hasMatch())
            continue;
        CompilerDiag d;
        d.line = m.captured(1).toInt();
        d.column = m.captured(2).toInt();
        d.isError = m.captured(3) == QLatin1String("error");
        d.message = m.captured(4).trimmed();
        if (d.line > 0 && d.message != QLatin1String("In file included from"))
            diags.push_back(d);
    }
    emit finished(diags, m_compiler);
}

} // namespace paw
