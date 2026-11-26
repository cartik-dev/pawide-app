#include "ProjectTemplates.h"

#include "explorer/FileOperations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>

namespace paw::ProjectTemplates {

namespace {

struct FileSpec {
    QString path;
    QString content;
};

QString identifier(const QString &name)
{
    QString s = name;
    s.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]")), QStringLiteral("_"));
    if (s.isEmpty() || s.at(0).isDigit())
        s.prepend(QLatin1Char('_'));
    return s;
}

QString crateName(const QString &name)
{
    QString s = name.toLower();
    s.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]")), QStringLiteral("-"));
    if (s.isEmpty() || !s.at(0).isLetter())
        s.prepend(QStringLiteral("app-"));
    return s;
}

QString fill(QString text, const QString &name)
{
    text.replace(QStringLiteral("@HTMLNAME@"), name.toHtmlEscaped());
    text.replace(QStringLiteral("@IDENT@"), identifier(name));
    text.replace(QStringLiteral("@CRATE@"), crateName(name));
    text.replace(QStringLiteral("@NAME@"), name);
    return text;
}

QString readme(const QString &extra)
{
    return QStringLiteral("# @NAME@\n\nProject created with Paw IDE.\n") + extra;
}

QList<FileSpec> filesFor(const QString &id)
{
    if (id == QLatin1String("empty")) {
        return {
            {"README.md", readme({})},
            {".gitignore", QStringLiteral(".DS_Store\nThumbs.db\n*.log\n")},
        };
    }
    if (id == QLatin1String("cpp")) {
        return {
            {"CMakeLists.txt", QStringLiteral(
                 "cmake_minimum_required(VERSION 3.16)\n"
                 "project(@IDENT@ VERSION 0.1.0 LANGUAGES CXX)\n\n"
                 "set(CMAKE_CXX_STANDARD 20)\n"
                 "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n"
                 "add_executable(@IDENT@ src/main.cpp)\n")},
            {"src/main.cpp", QStringLiteral(
                 "#include <iostream>\n\n"
                 "int main()\n{\n    std::cout << \"Hello from @NAME@!\" << std::endl;\n    return 0;\n}\n")},
            {"README.md", readme(QStringLiteral("\n## Build\n\n```bash\ncmake -S . -B build\ncmake --build build\n```\n"))},
            {".gitignore", QStringLiteral("build/\nout/\n*.o\n*.obj\n*.exe\n")},
        };
    }
    if (id == QLatin1String("python")) {
        return {
            {"main.py", QStringLiteral(
                 "def greet(name: str) -> str:\n    return f\"Hello, {name}!\"\n\n\n"
                 "def main() -> None:\n    print(greet(\"@NAME@\"))\n\n\n"
                 "if __name__ == \"__main__\":\n    main()\n")},
            {"tests/test_main.py", QStringLiteral(
                 "import unittest\n\nfrom main import greet\n\n\n"
                 "class GreetTest(unittest.TestCase):\n    def test_greet(self):\n"
                 "        self.assertEqual(greet(\"Paw\"), \"Hello, Paw!\")\n\n\n"
                 "if __name__ == \"__main__\":\n    unittest.main()\n")},
            {"requirements.txt", QStringLiteral("# Add your dependencies here, one per line.\n")},
            {"README.md", readme(QStringLiteral("\n## Run\n\n```bash\npython main.py\npython -m unittest discover -s tests\n```\n"))},
            {".gitignore", QStringLiteral("__pycache__/\n*.pyc\n.venv/\nvenv/\n.env\n")},
        };
    }
    if (id == QLatin1String("web")) {
        return {
            {"index.html", QStringLiteral(
                 "<!doctype html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n"
                 "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
                 "  <title>@HTMLNAME@</title>\n  <link rel=\"stylesheet\" href=\"css/style.css\">\n</head>\n<body>\n"
                 "  <main>\n    <h1>@HTMLNAME@</h1>\n    <p id=\"message\"></p>\n  </main>\n"
                 "  <script src=\"js/main.js\"></script>\n</body>\n</html>\n")},
            {"css/style.css", QStringLiteral(
                 "body {\n  margin: 0;\n  min-height: 100vh;\n  display: grid;\n  place-items: center;\n"
                 "  font-family: system-ui, sans-serif;\n  background: #0b0b0f;\n  color: #f4f4f5;\n}\n")},
            {"js/main.js", QStringLiteral(
                 "document.getElementById('message').textContent = 'Hello from Paw IDE!';\n")},
            {"README.md", readme(QStringLiteral("\nOpen `index.html` in a browser.\n"))},
            {".gitignore", QStringLiteral("node_modules/\ndist/\n.env\n")},
        };
    }
    if (id == QLatin1String("rust")) {
        return {
            {"Cargo.toml", QStringLiteral("[package]\nname = \"@CRATE@\"\nversion = \"0.1.0\"\nedition = \"2021\"\n\n[dependencies]\n")},
            {"src/main.rs", QStringLiteral("fn main() {\n    println!(\"Hello from @NAME@!\");\n}\n")},
            {"README.md", readme(QStringLiteral("\n## Run\n\n```bash\ncargo run\n```\n"))},
            {".gitignore", QStringLiteral("/target\n")},
        };
    }
    if (id == QLatin1String("lua")) {
        return {
            {"main.lua", QStringLiteral(
                 "local greeter = require(\"src.greeter\")\n\nprint(greeter.greet(\"@NAME@\"))\n")},
            {"src/greeter.lua", QStringLiteral(
                 "local M = {}\n\nfunction M.greet(name)\n  return \"Hello, \" .. name .. \"!\"\nend\n\nreturn M\n")},
            {"README.md", readme(QStringLiteral("\n## Run\n\n```bash\nlua main.lua\n```\n"))},
            {".gitignore", QStringLiteral("*.luac\n")},
        };
    }
    return {};
}

} // namespace

QList<TemplateInfo> all()
{
    return {
        {"empty", "Empty Project", "A blank folder with a README and .gitignore.", "README.md"},
        {"cpp", "C++ Project", "CMake-based C++20 console application.", "src/main.cpp"},
        {"python", "Python Project", "Python script with unit tests.", "main.py"},
        {"web", "Web Project", "HTML, CSS and JavaScript starter page.", "index.html"},
        {"rust", "Rust Project", "Cargo binary crate.", "src/main.rs"},
        {"lua", "Lua Project", "Lua program with a small module.", "main.lua"},
    };
}

bool validateProjectName(const QString &name, QString *error)
{
    if (!fileops::validateName(name, error))
        return false;
    if (name != name.trimmed()) {
        if (error)
            *error = QStringLiteral("The name cannot start or end with a space.");
        return false;
    }
    return true;
}

CreateResult create(const QString &templateId, const QString &nameIn, const QString &parentDir)
{
    CreateResult r;
    const QString name = nameIn.trimmed();
    if (!validateProjectName(name, &r.error))
        return r;
    const QList<FileSpec> files = filesFor(templateId);
    if (files.isEmpty()) {
        r.error = QStringLiteral("Unknown project template '%1'.").arg(templateId);
        return r;
    }
    const QFileInfo parent(parentDir);
    if (!parent.exists() || !parent.isDir()) {
        r.error = QStringLiteral("The selected location does not exist.");
        return r;
    }
    const QString projectPath = QDir(parent.absoluteFilePath()).filePath(name);
    const QFileInfo target(projectPath);
    bool createdFolder = false;
    if (target.exists()) {
        if (!target.isDir() || !QDir(projectPath).isEmpty(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
            r.error = QStringLiteral("'%1' already exists and is not an empty folder.").arg(projectPath);
            return r;
        }
    } else {
        if (!QDir().mkpath(projectPath)) {
            r.error = QStringLiteral("Cannot create the folder '%1'.").arg(projectPath);
            return r;
        }
        createdFolder = true;
    }

    for (const FileSpec &f : files) {
        const QString full = QDir(projectPath).filePath(f.path);
        QDir().mkpath(QFileInfo(full).absolutePath());
        QFile out(full);
        if (!out.open(QIODevice::WriteOnly) || out.write(fill(f.content, name).toUtf8()) < 0) {
            r.error = QStringLiteral("Cannot write '%1': %2").arg(f.path, out.errorString());
            out.close();
            if (createdFolder)
                QDir(projectPath).removeRecursively();
            return r;
        }
    }

    r.ok = true;
    r.projectPath = projectPath;
    for (const TemplateInfo &t : all()) {
        if (t.id == templateId)
            r.mainFile = QDir(projectPath).filePath(t.mainFile);
    }
    return r;
}

} // namespace paw::ProjectTemplates
