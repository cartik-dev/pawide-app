#include "core/AppContext.h"
#include "database/Database.h"
#include "editor/CodeEditor.h"
#include "editor/EditorArea.h"
#include "explorer/ExplorerWidget.h"
#include "project/ProjectTemplates.h"
#include "settings/Settings.h"
#include "ui/CommandPalette.h"
#include "ui/MainWindow.h"
#include "ui/WelcomeWidget.h"

#include <QAbstractButton>
#include <QDir>
#include <QFile>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeView>
#include <QtTest>

using namespace paw;

static void writeFile(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

static QByteArray readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return f.readAll();
}

// Clicks the given standard button of the next message box that appears.
static void clickMessageBoxLater(QMessageBox::StandardButton which)
{
    QTimer::singleShot(150, [which] {
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (auto *mb = qobject_cast<QMessageBox *>(w); mb && mb->isVisible()) {
                if (auto *b = mb->button(which)) {
                    b->click();
                    return;
                }
            }
        }
    });
}

static QStringList visibleNames(ExplorerWidget *ex, const QModelIndex &parent)
{
    QStringList names;
    auto *m = ex->model();
    for (int i = 0; i < m->rowCount(parent); ++i)
        names << m->fileName(m->index(i, 0, parent));
    return names;
}

class TstMainWindow : public QObject {
    Q_OBJECT
    QTemporaryDir m_tmp;
    QString project() const { return QDir(m_tmp.path()).filePath("MyProject"); }

private slots:
    void initTestCase()
    {
        setDatabasePath(":memory:"); // never touch the real user database
        Settings::instance().setShowHiddenFiles(true);
        // A project with hidden, extension-less and nested entries, like the spec example.
        writeFile(project() + "/src/main.cpp", "int main() {}\n");
        writeFile(project() + "/src/app.cpp", "// app\n");
        writeFile(project() + "/assets/logo.png", QByteArray("\x89PNG\r\n\x1a\n\x00\x00", 10));
        writeFile(project() + "/config/settings.json", "{}\n");
        writeFile(project() + "/.gitignore", "build/\n");
        writeFile(project() + "/.env", "KEY=1\n");
        writeFile(project() + "/.editorconfig", "root = true\n");
        writeFile(project() + "/Dockerfile", "FROM scratch\n");
        writeFile(project() + "/Makefile", "all:\n");
        writeFile(project() + "/README.md", "# Hi\n");
        writeFile(project() + "/CMakeLists.txt", "project(x)\n");
        writeFile(project() + "/.config/nested/.secret", "x\n");
    }

    void appStartsWithWelcomeScreen()
    {
        MainWindow w;
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        QVERIFY(w.windowTitle().contains("Paw IDE"));
        QVERIFY(w.welcome()->isVisible());
        QVERIFY(w.projectPath().isEmpty());
    }

    void openFolderShowsNestedAndHiddenFiles()
    {
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(project()));
        QCOMPARE(w.projectPath(), QDir::cleanPath(project()));
        QVERIFY(w.editorArea()->isVisible());
        QVERIFY(w.windowTitle().contains("MyProject"));

        ExplorerWidget *ex = w.explorer();
        const QModelIndex root = ex->view()->rootIndex();
        QVERIFY(root.isValid());
        QTRY_VERIFY(ex->model()->rowCount(root) >= 11);
        const QStringList names = visibleNames(ex, root);
        for (const char *expected : {"src", "assets", "config", ".gitignore", ".env", ".editorconfig", "Dockerfile",
                                     "Makefile", "README.md", "CMakeLists.txt", ".config"})
            QVERIFY2(names.contains(expected), expected);

        // nested directory
        const QModelIndex src = ex->model()->index(project() + "/src");
        ex->view()->expand(src);
        QTRY_COMPARE(ex->model()->rowCount(src), 2);
        QVERIFY(visibleNames(ex, src).contains("main.cpp"));

        // deeply nested hidden entries (parents have to be expanded to be loaded)
        const QModelIndex cfg = ex->model()->index(project() + "/.config");
        ex->view()->expand(cfg);
        QTRY_COMPARE(ex->model()->rowCount(cfg), 1);
        const QModelIndex nested = ex->model()->index(project() + "/.config/nested");
        ex->view()->expand(nested);
        QTRY_COMPARE(ex->model()->rowCount(nested), 1);
        QCOMPARE(visibleNames(ex, nested).first(), QString(".secret"));
    }

    void hiddenFilesCanBeToggled()
    {
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(project()));
        ExplorerWidget *ex = w.explorer();
        QTRY_VERIFY(visibleNames(ex, ex->view()->rootIndex()).contains(".env"));
        Settings::instance().setShowHiddenFiles(false);
        QTRY_VERIFY(!visibleNames(ex, ex->view()->rootIndex()).contains(".env"));
        QVERIFY(visibleNames(ex, ex->view()->rootIndex()).contains("Dockerfile")); // no extension != hidden
        Settings::instance().setShowHiddenFiles(true);
        QTRY_VERIFY(visibleNames(ex, ex->view()->rootIndex()).contains(".env"));
    }

    void openEditAndSavePreservesEncodingAndLineEndings()
    {
        const QString path = project() + "/src/crlf.txt";
        const QByteArray original = QByteArray("\xEF\xBB\xBF") + QString::fromUtf8("Привет\r\nмир\r\n").toUtf8();
        writeFile(path, original);

        MainWindow w;
        w.show();
        QVERIFY(w.openProject(project()));
        w.openFile(path);
        QTRY_COMPARE(w.editorArea()->count(), 1);
        CodeEditor *e = w.editorArea()->currentEditor();
        QVERIFY(e);
        QCOMPARE(e->encoding(), QString("UTF-8 BOM"));
        QCOMPARE(e->lineEnding(), LineEnding::CRLF);
        QCOMPARE(e->toPlainText(), QString::fromUtf8("Привет\nмир\n"));
        QVERIFY(!e->document()->isModified());

        e->moveCursor(QTextCursor::End);
        e->insertPlainText("added\n");
        QVERIFY(e->document()->isModified());
        QVERIFY(w.editorArea()->hasUnsavedChanges());
        QVERIFY(w.editorArea()->saveCurrent());
        QVERIFY(!e->document()->isModified());

        const QByteArray saved = readFile(path);
        QVERIFY(saved.startsWith("\xEF\xBB\xBF"));
        QCOMPARE(saved, original + "added\r\n");
        QVERIFY(!saved.contains("\r\r"));
        QCOMPARE(saved.count('\n'), saved.count("\r\n")); // no bare LF slipped in
    }

    void sameFileIsOpenedOnlyOnce()
    {
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(project()));
        w.openFile(project() + "/README.md");
        QTRY_COMPARE(w.editorArea()->count(), 1);
        w.openFile(project() + "/README.md");
        w.openFile(project() + "/.env");
        QTRY_COMPARE(w.editorArea()->count(), 2);
        QCOMPARE(w.editorArea()->editorForPath(project() + "/.env")->language().id, QString("dotenv"));
    }

    void hiddenAndExtensionlessFilesOpenAsText()
    {
        MainWindow w;
        w.show();
        w.openFile(project() + "/Dockerfile");
        w.openFile(project() + "/.gitignore");
        QTRY_COMPARE(w.editorArea()->count(), 2);
        QCOMPARE(w.editorArea()->editorForPath(project() + "/Dockerfile")->toPlainText(), QString("FROM scratch\n"));
    }

    void unsavedChangesBlockProjectClose()
    {
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(project()));
        w.openFile(project() + "/README.md");
        QTRY_COMPARE(w.editorArea()->count(), 1);
        w.editorArea()->currentEditor()->insertPlainText("edit");
        clickMessageBoxLater(QMessageBox::Cancel);
        QVERIFY(!w.closeProject()); // cancelled: nothing lost
        QCOMPARE(w.projectPath(), QDir::cleanPath(project()));
        QCOMPARE(w.editorArea()->count(), 1);
        QVERIFY(w.editorArea()->hasUnsavedChanges());
        clickMessageBoxLater(QMessageBox::Discard);
        QVERIFY(w.closeProject());
        QVERIFY(w.projectPath().isEmpty());
        QVERIFY(w.welcome()->isVisible());
        QCOMPARE(readFile(project() + "/README.md"), QByteArray("# Hi\n")); // discard really discarded
    }

    void newProjectFromTemplateOpensAndIsRemembered()
    {
        QTemporaryDir tmp;
        const CreateResult r = ProjectTemplates::create("lua", "LuaDemo", tmp.path());
        QVERIFY2(r.ok, qPrintable(r.error));
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(r.projectPath));
        w.openFile(r.mainFile);
        QTRY_COMPARE(w.editorArea()->count(), 1);
        QCOMPARE(w.editorArea()->currentEditor()->language().name, QString("Lua"));
        const auto recent = appDatabase().recentProjects();
        QVERIFY(!recent.isEmpty());
        QCOMPARE(recent.first().path, QDir::cleanPath(r.projectPath));
        QCOMPARE(recent.first().name, QString("LuaDemo"));
    }

    void invalidFolderIsRejected()
    {
        MainWindow w;
        w.show();
        clickMessageBoxLater(QMessageBox::Ok);
        QVERIFY(!w.openProject(m_tmp.filePath("does-not-exist")));
        QVERIFY(w.projectPath().isEmpty());
    }

    void sameProjectIsNotOpenedInTwoWindows()
    {
        MainWindow a, b;
        a.show();
        b.show();
        QVERIFY(a.openProject(project()));
        QVERIFY(b.openProject(project())); // raises window a instead
        QVERIFY(b.projectPath().isEmpty());
        QCOMPARE(MainWindow::windows().size(), 2);
    }

    void explorerOperations()
    {
        QTemporaryDir tmp;
        MainWindow w;
        w.show();
        QVERIFY(w.openProject(tmp.path()));
        ExplorerWidget *ex = w.explorer();
        QVERIFY(ex->createFolderIn(tmp.path(), "sub"));
        QVERIFY(ex->createFileIn(tmp.path() + "/sub", ".hiddenfile"));
        QVERIFY(QFileInfo(tmp.path() + "/sub/.hiddenfile").isFile());
        QTRY_COMPARE(w.editorArea()->count(), 1); // created files are opened
        QVERIFY(ex->renameEntry(tmp.path() + "/sub/.hiddenfile", "renamed.txt"));
        QVERIFY(QFileInfo(tmp.path() + "/sub/renamed.txt").isFile());
        QCOMPARE(w.editorArea()->currentEditor()->filePath(), tmp.path() + "/sub/renamed.txt"); // tab follows rename
        QVERIFY(ex->duplicateEntry(tmp.path() + "/sub/renamed.txt"));
        QVERIFY(QFileInfo(tmp.path() + "/sub/renamed copy.txt").isFile());
        ex->copyEntries({tmp.path() + "/sub"});
        QVERIFY(ex->pasteInto(tmp.path()));
        QVERIFY(QFileInfo(tmp.path() + "/sub copy/renamed.txt").isFile());
        QVERIFY(ex->deleteEntry(tmp.path() + "/sub", false));
        QVERIFY(!QFileInfo::exists(tmp.path() + "/sub"));
        QTRY_COMPARE(w.editorArea()->count(), 0); // tab of the deleted file is closed
        QVERIFY(!ex->deleteEntry(ex->rootPath(), false)); // the project root itself cannot be deleted
    }

    void editorTypingBehaviour()
    {
        CodeEditor e;
        e.show();
        QTest::keyClicks(&e, "(");
        QCOMPARE(e.toPlainText(), QString("()"));
        QTest::keyClicks(&e, ")"); // types over the auto-inserted bracket
        QCOMPARE(e.toPlainText(), QString("()"));
        QCOMPARE(e.textCursor().position(), 2);
        e.clear();
        QTest::keyClicks(&e, "{");
        QTest::keyClick(&e, Qt::Key_Return);
        QCOMPARE(e.toPlainText(), QString("{\n    \n}"));
        QCOMPARE(e.currentLine(), 2);
        QCOMPARE(e.currentColumn(), 5);
        e.clear();
        QTest::keyClicks(&e, "x = 1");
        QTest::keyClick(&e, Qt::Key_Return);
        QCOMPARE(e.toPlainText(), QString("x = 1\n"));
        e.clear();
        QTest::keyClicks(&e, "(");
        QTest::keyClick(&e, Qt::Key_Backspace); // removes the pair
        QCOMPARE(e.toPlainText(), QString());
        e.clear();
        QTest::keyClicks(&e, "don't"); // apostrophe inside a word is not paired
        QCOMPARE(e.toPlainText(), QString("don't"));
    }

    void commandPalette()
    {
        int score = 0;
        QVERIFY(CommandPalette::fuzzyMatch("ofl", "File: Open Folder…", &score));
        QVERIFY(!CommandPalette::fuzzyMatch("xyz", "File: Open Folder…"));
        MainWindow w;
        w.show();
        bool found = false;
        for (QAction *a : w.allActions())
            if (a->text().contains("Command Palette")) {
                found = true;
                QCOMPARE(a->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
            }
        QVERIFY(found);
        w.palette()->showItems({{"File: Open Folder…", "", [] {}, true}, {"View: Toggle Panel", "", [] {}, true}}, "Type");
        QCOMPARE(w.palette()->visibleCount(), 2);
        w.palette()->setQuery("panel");
        QCOMPARE(w.palette()->visibleCount(), 1);
    }

    void recentProjectsSurviveButFoldersAreNotDeleted()
    {
        appDatabase().addRecentProject(project());
        appDatabase().clearRecentProjects();
        QVERIFY(QDir(project()).exists());
    }
};

QTEST_MAIN(TstMainWindow)
#include "tst_mainwindow.moc"
