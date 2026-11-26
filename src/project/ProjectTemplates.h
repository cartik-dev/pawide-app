#pragma once
#include <QList>
#include <QString>

namespace paw {

struct TemplateInfo {
    QString id;
    QString name;
    QString description;
    QString mainFile; // relative path opened after creation
};

struct CreateResult {
    bool ok = false;
    QString error;
    QString projectPath;
    QString mainFile; // absolute path
};

namespace ProjectTemplates {
QList<TemplateInfo> all();
bool validateProjectName(const QString &name, QString *error);
// Creates `<parentDir>/<name>` and fills it with the real files of the template.
CreateResult create(const QString &templateId, const QString &name, const QString &parentDir);
} // namespace ProjectTemplates

} // namespace paw
