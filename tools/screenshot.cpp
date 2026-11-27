// Developer tool: renders the main window to a PNG (works with QT_QPA_PLATFORM=offscreen).
//   pawide_screenshot <out.png> welcome
//   pawide_screenshot <out.png> project <folder> [file-to-open] [palette]
//   pawide_screenshot <out.png> plugins <folder>                 Extensions side view
//   pawide_screenshot <out.png> run <folder> [run]               Run & Debug side view (opt. start a workflow)
//   pawide_screenshot <out.png> suspect <folder> <file>          amber "not sure" state (before the 15 s check)
//   pawide_screenshot <out.png> confirmed <folder> <file>        after 15 s: sandbox verdict card + Problems
#include "core/AppContext.h"
#include "core/PluginManager.h"
#include "ui/CommandPalette.h"
#include "ui/Icons.h"
#include "ui/MainWindow.h"
#include "ui/RunPanel.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QEventLoop>
#include <QPainter>
#include <QTimer>

static void wait(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc < 3)
        return 2;
    paw::setDatabasePath(QStringLiteral(":memory:"));
    // Language packs under PAWIDE_PLUGINS_DIR are activated before the UI
    // exists, so screenshots can capture a translated interface.
    paw::PluginManager::instance().scan();
    paw::PluginManager::instance().activate();
    paw::theme::apply(app);
    paw::MainWindow w;
    w.resize(1320, 820);
    w.show();
    const QString mode = QString::fromLocal8Bit(argv[2]);
    if (mode == QLatin1String("project") && argc >= 4) {
        w.openProject(QString::fromLocal8Bit(argv[3]));
        wait(400);
        if (argc >= 5)
            w.openFile(QString::fromLocal8Bit(argv[4]));
        if (argc >= 6) {
            w.palette()->showItems({{"File: Open Folder…", "Ctrl+Shift+O", {}, true}, {"File: New Project…", "", {}, true},
                                    {"View: Toggle Panel", "Ctrl+J", {}, true}}, "Type a command");
            w.palette()->show();
        }
    } else if (mode == QLatin1String("plugins") && argc >= 4) {
        w.openProject(QString::fromLocal8Bit(argv[3]));
        wait(400);
        w.showView(QStringLiteral("extensions"));
    } else if (mode == QLatin1String("run") && argc >= 4) {
        w.openProject(QString::fromLocal8Bit(argv[3]));
        wait(400);
        w.showView(QStringLiteral("run"));
        w.runPanel()->runWorkflow(0); // Build & Run for a CMake project
        wait(1400);                    // let some output stream in
    } else if ((mode == QLatin1String("suspect") || mode == QLatin1String("confirmed") || mode == QLatin1String("sure"))
               && argc >= 5) {
        w.openProject(QString::fromLocal8Bit(argv[3]));
        wait(400);
        w.showView(QStringLiteral("explorer"));
        w.openFile(QString::fromLocal8Bit(argv[4]));
        // 700 ms debounce + analysis; the 15 s timer then triggers the sandbox.
        wait(mode == QLatin1String("suspect") || mode == QLatin1String("sure") ? 2500 : 18000);
    }
    wait(900);
    QPixmap base = w.grab();
    // A QDialog is a separate top-level window: composite it onto the main
    // window grab manually, at its position relative to the parent.
    if (argc >= 6 && mode == QLatin1String("project")) {
        const QPixmap pal = w.palette()->grab();
        QPainter painter(&base);
        painter.drawPixmap(w.palette()->pos(), pal);
        painter.end();
    }
    return base.save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
