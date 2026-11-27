#pragma once
#include "core/CodeIssue.h"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>

namespace paw {

struct CompilerDiag {
    int line = 0;
    int column = 0;
    bool isError = false;
    QString message;
};

// Runs the current file through the configured compiler in an isolated
// temporary directory ("sandbox"): the buffer is copied there, nothing in the
// project or on disk is saved or modified, and the whole directory is deleted
// when the check ends.
class SandboxVerifier : public QObject {
    Q_OBJECT
public:
    explicit SandboxVerifier(QObject *parent = nullptr);

    // Returns false immediately when no C/C++ compiler is available.
    bool verify(const QString &sourceText, const QString &fileName, const QString &languageId);

    static QString findCompiler(); // g++ / clang++ / c++ or empty

signals:
    void finished(const QList<paw::CompilerDiag> &diagnostics, const QString &compiler);
    void progress(const QString &stage);

private slots:
    void collectResult();

private:
    QProcess *m_proc = nullptr;
    QTemporaryDir *m_sandbox = nullptr; // lives until the check ends, then is wiped
    QTimer m_timeout;
    QString m_compiler;
};

} // namespace paw
