#include "CodeIssue.h"

#include <QMap>
#include <QRegularExpression>
#include <QSet>

namespace paw {

QList<CodeIssue> SuspicionAnalyzer::analyze(const QString &text, const QString &languageId)
{
    QList<CodeIssue> issues;
    // Heuristics target C-style syntax; other languages stay untouched for now.
    static const QSet<QString> languages = {QStringLiteral("cpp"), QStringLiteral("c"), QStringLiteral("java"),
                                            QStringLiteral("javascript"), QStringLiteral("typescript")};
    if (!languages.contains(languageId))
        return issues;

    const QStringList lines = text.split(QLatin1Char('\n'));
    // Declared locals: name -> line (1-based). Used later to detect unused ones.
    QMap<QString, int> declared;
    QRegularExpression declRx(QStringLiteral(
        "^\\s*(?:const\\s+)?(?:int|double|float|bool|char|auto|long|short|unsigned|size_t)\\s+([A-Za-z_]\\w*)\\s*(?:=|;)"));
    // `if (x = ...)` / `while (x = ...)` — assignment used as a condition.
    QRegularExpression assignInCondRx(QStringLiteral("\\b(if|while)\\s*\\(\\s*[A-Za-z_]\\w*\\s*=\\s*[^=]"));

    for (int i = 0; i < lines.size(); ++i) {
        const QString raw = lines.at(i);
        const QString trimmed = raw.trimmed();
        if (trimmed.startsWith(QLatin1String("//")) || trimmed.startsWith(QLatin1String("*"))
            || trimmed.startsWith(QLatin1String("/*")))
            continue;
        const QString line = raw.section(QStringLiteral("//"), 0, 0); // ignore trailing comments

        const auto cond = assignInCondRx.match(line);
        if (cond.hasMatch()) {
            CodeIssue issue;
            issue.line = i + 1;
            issue.column = cond.capturedStart(2) + 1;
            issue.length = cond.captured(2).size();
            issue.message = QStringLiteral("Assignment used as a condition — did you mean '=='? This is always wrong.");
            issue.severity = CodeIssue::Severity::SureError;
            issues.push_back(issue);
            continue;
        }

        const auto decl = declRx.match(line);
        if (decl.hasMatch())
            declared.insert(decl.captured(1), i + 1);
    }

    // Suspect: a declared local that is never referenced anywhere else in the file.
    for (auto it = declared.cbegin(); it != declared.cend(); ++it) {
        QRegularExpression wordRx(QStringLiteral("\\b%1\\b").arg(it.key()));
        int uses = 0;
        for (const QString &raw : lines) {
            auto mit = wordRx.globalMatch(raw);
            while (mit.hasNext()) {
                mit.next();
                ++uses;
            }
        }
        if (uses > 1)
            continue;
        const QString &declLine = lines.value(it.value() - 1);
        const int col = declLine.indexOf(it.key());
        CodeIssue issue;
        issue.line = it.value();
        issue.column = col >= 0 ? col + 1 : 1;
        issue.length = qMax(1, static_cast<int>(it.key().size()));
        issue.message = QStringLiteral("'%1' is declared but never used — possible dead code.").arg(it.key());
        issue.severity = CodeIssue::Severity::Suspect;
        issues.push_back(issue);
    }
    return issues;
}

} // namespace paw
