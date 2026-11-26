#pragma once
#include <QString>

namespace paw {
struct LanguageInfo {
    QString id;   // e.g. "cpp"
    QString name; // e.g. "C++"
};

namespace Language {
LanguageInfo forFile(const QString &path);
} // namespace Language
} // namespace paw
