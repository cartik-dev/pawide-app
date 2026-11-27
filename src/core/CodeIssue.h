#pragma once
#include <QList>
#include <QString>

namespace paw {

// A single finding produced by the SuspicionAnalyzer or by the sandbox check.
struct CodeIssue {
    enum class Severity { SureError, Suspect, Confirmed };
    int line = 0;        // 1-based
    int column = 0;      // 1-based
    int length = 0;      // highlighted characters
    QString message;
    Severity severity = Severity::Suspect;
};

// Lightweight heuristic scanner that runs on every edit (debounced).
// It only flags patterns it is *not certain* about as Suspect; the tiny set of
// constructs that are always wrong is flagged as SureError and shown right away.
class SuspicionAnalyzer {
public:
    static QList<CodeIssue> analyze(const QString &text, const QString &languageId);
};

} // namespace paw
