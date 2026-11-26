#include "database/Database.h"

#include <QDir>
#include <QTemporaryDir>
#include <QtTest>

using namespace paw;

class TstDatabase : public QObject {
    Q_OBJECT
private slots:
    void settingsRoundTrip()
    {
        Database db;
        QVERIFY(db.open(":memory:"));
        QCOMPARE(db.value("missing", "dflt").toString(), QString("dflt"));
        QSignalSpy spy(&db, &Database::valueChanged);
        db.setValue("a", "1");
        db.setValue("a", "2"); // update, not duplicate
        QCOMPARE(db.value("a").toString(), QString("2"));
        QCOMPARE(spy.count(), 2);
    }
    void persistsOnDisk()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("test.db");
        {
            Database db;
            QVERIFY(db.open(file));
            db.setValue("theme", "violet");
            db.addRecentProject("/some/project");
        }
        Database db2;
        QVERIFY(db2.open(file));
        QCOMPARE(db2.value("theme").toString(), QString("violet"));
        QCOMPARE(db2.recentProjects().size(), 1);
    }
    void recentProjectsOrderAndDedupe()
    {
        Database db;
        QVERIFY(db.open(":memory:"));
        db.addRecentProject("/p/one");
        QTest::qWait(5);
        db.addRecentProject("/p/two");
        QTest::qWait(5);
        db.addRecentProject("/p/one"); // moves to the top, no duplicate
        const auto list = db.recentProjects();
        QCOMPARE(list.size(), 2);
        QCOMPARE(list.first().path, QString("/p/one"));
        QCOMPARE(list.first().name, QString("one"));
        QVERIFY(list.first().lastOpened.isValid());
    }
    void pathsAreNormalised()
    {
        Database db;
        QVERIFY(db.open(":memory:"));
        db.addRecentProject("C:/Users/User/Projects/MyProject/"); // trailing slash is trimmed
        db.addRecentProject("C:/Users/User/Projects/MyProject");  // same project, no duplicate
#ifdef Q_OS_WIN
        db.addRecentProject("C:\\Users\\User\\Projects\\MyProject"); // native Windows form
#endif
        QCOMPARE(db.recentProjects().size(), 1);
        QCOMPARE(db.recentProjects().first().name, QString("MyProject"));
        QCOMPARE(db.recentProjects().first().path, QString("C:/Users/User/Projects/MyProject"));
    }
    void removeAndClearNeverTouchFolders()
    {
        QTemporaryDir tmp;
        QVERIFY(QDir(tmp.path()).mkpath("proj"));
        const QString proj = QDir(tmp.path()).filePath("proj");
        Database db;
        QVERIFY(db.open(":memory:"));
        db.addRecentProject(proj);
        db.addRecentProject("/other");
        QSignalSpy spy(&db, &Database::recentProjectsChanged);
        db.removeRecentProject("/other");
        QCOMPARE(db.recentProjects().size(), 1);
        db.clearRecentProjects();
        QVERIFY(db.recentProjects().isEmpty());
        QVERIFY(QDir(proj).exists()); // history only, folders are untouched
        QCOMPARE(spy.count(), 2);
    }
    void historyIsLimited()
    {
        Database db;
        QVERIFY(db.open(":memory:"));
        for (int i = 0; i < 30; ++i)
            db.addRecentProject(QString("/p/%1").arg(i));
        QCOMPARE(db.recentProjects().size(), 20);
    }
};

QTEST_GUILESS_MAIN(TstDatabase)
#include "tst_database.moc"
