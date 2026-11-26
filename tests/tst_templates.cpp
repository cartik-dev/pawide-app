#include "project/ProjectTemplates.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace paw;

class TstTemplates : public QObject {
    Q_OBJECT
private slots:
    void allTemplatesCreateRealFiles_data()
    {
        QTest::addColumn<QString>("id");
        for (const auto &t : ProjectTemplates::all())
            QTest::newRow(qPrintable(t.id)) << t.id;
    }
    void allTemplatesCreateRealFiles()
    {
        QFETCH(QString, id);
        QTemporaryDir tmp;
        const CreateResult r = ProjectTemplates::create(id, "Demo App", tmp.path());
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY(QFileInfo(r.projectPath).isDir());
        QVERIFY(QFileInfo(r.mainFile).isFile());
        int files = 0;
        QDirIterator it(r.projectPath, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QFile f(it.next());
            QVERIFY(f.open(QIODevice::ReadOnly));
            QVERIFY2(f.size() > 0, qPrintable(f.fileName() + " is empty")); // no fake empty files
            ++files;
        }
        QVERIFY(files >= 2);
        QVERIFY(QFileInfo(QDir(r.projectPath).filePath("README.md")).isFile());
        QVERIFY(QFileInfo(QDir(r.projectPath).filePath(".gitignore")).isFile()); // hidden file is created too
    }
    void placeholdersAreSubstituted()
    {
        QTemporaryDir tmp;
        auto content = [&](const QString &rel, const CreateResult &r) {
            QFile f(QDir(r.projectPath).filePath(rel));
            f.open(QIODevice::ReadOnly);
            return QString::fromUtf8(f.readAll());
        };
        const CreateResult cpp = ProjectTemplates::create("cpp", "My Cool-App", tmp.path());
        QVERIFY(cpp.ok);
        QVERIFY(content("CMakeLists.txt", cpp).contains("project(My_Cool_App "));
        QVERIFY(!content("src/main.cpp", cpp).contains("@"));
        const CreateResult rust = ProjectTemplates::create("rust", "My Crate", tmp.path());
        QVERIFY(content("Cargo.toml", rust).contains("name = \"my-crate\""));
        const CreateResult web = ProjectTemplates::create("web", "A&B", tmp.path());
        QVERIFY(content("index.html", web).contains("A&amp;B"));
    }
    void invalidNamesRejected_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("empty") << "";
        QTest::newRow("slash") << "a/b";
        QTest::newRow("backslash") << "a\\b";
        QTest::newRow("colon") << "a:b";
        QTest::newRow("reserved") << "CON";
        QTest::newRow("reserved-ext") << "nul.txt";
        QTest::newRow("trailing-dot") << "proj.";
        QTest::newRow("dotdot") << "..";
        QTest::newRow("star") << "a*b";
    }
    void invalidNamesRejected()
    {
        QFETCH(QString, name);
        QTemporaryDir tmp;
        const CreateResult r = ProjectTemplates::create("empty", name, tmp.path());
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
        QVERIFY(QDir(tmp.path()).isEmpty()); // nothing was created
    }
    void existingFolderHandling()
    {
        QTemporaryDir tmp;
        QVERIFY(QDir(tmp.path()).mkpath("full"));
        QFile f(QDir(tmp.path()).filePath("full/keep.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("precious");
        f.close();
        QVERIFY(!ProjectTemplates::create("empty", "full", tmp.path()).ok); // never overwrites
        QVERIFY(QFileInfo(f.fileName()).isFile());
        QVERIFY(QDir(tmp.path()).mkpath("blank"));
        QVERIFY(ProjectTemplates::create("empty", "blank", tmp.path()).ok); // empty folder is fine
    }
    void unknownTemplateAndBadLocation()
    {
        QTemporaryDir tmp;
        QVERIFY(!ProjectTemplates::create("nope", "x", tmp.path()).ok);
        QVERIFY(!ProjectTemplates::create("empty", "x", tmp.filePath("missing-parent")).ok);
    }
};

QTEST_APPLESS_MAIN(TstTemplates)
#include "tst_templates.moc"
