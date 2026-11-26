#include "Theme.h"

#include "ui/Icons.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

namespace paw::theme {

static QString hex(const QColor &c) { return c.name(QColor::HexRgb); }

QString styleSheet()
{
    QString s = QStringLiteral(R"(
QWidget { background: %bgWindow%; color: %text%; selection-background-color: %selection%; selection-color: #ffffff; }
QMainWindow, QDialog { background: %bgWindow%; }
QToolTip { background: %bgSideBar%; color: %text%; border: 1px solid %border%; padding: 4px 6px; }
QMenuBar { background: %bgActivity%; border-bottom: 1px solid %border%; }
QMenuBar::item { padding: 5px 10px; background: transparent; }
QMenuBar::item:selected { background: %border%; }
QMenu { background: %bgSideBar%; border: 1px solid %border%; padding: 4px; }
QMenu::item { padding: 6px 26px 6px 22px; border-radius: 3px; }
QMenu::item:selected { background: %accent%; color: #ffffff; }
QMenu::item:disabled { color: #55555d; }
QMenu::separator { height: 1px; background: %border%; margin: 4px 8px; }
QStatusBar { background: %accent%; color: #ffffff; }
QStatusBar QLabel { background: transparent; color: #ffffff; padding: 0 8px; }
QStatusBar::item { border: none; }
QSplitter::handle { background: %border%; }
QSplitter::handle:horizontal { width: 1px; }
QSplitter::handle:vertical { height: 1px; }
QTabWidget::pane { border: none; }
QTabBar { background: %bgActivity%; qproperty-drawBase: 0; }
QTabBar::tab { background: %bgActivity%; color: %textMuted%; padding: 8px 14px; border-right: 1px solid %border%; min-width: 60px; }
QTabBar::tab:selected { background: %bgEditor%; color: %text%; border-top: 2px solid %accent%; }
QTabBar::tab:hover:!selected { background: %bgSideBar%; }
QTabBar::close-button { subcontrol-position: right; image: url(:/icons/close.svg); border-radius: 3px; margin: 2px; }
QTabBar::close-button:hover { image: url(:/icons/close-hover.svg); background: %border%; }
QPlainTextEdit { background: %bgEditor%; color: %text%; border: none; }
QTreeView, QListWidget, QListView { background: %bgSideBar%; border: none; outline: 0; }
QTreeView::item, QListWidget::item { padding: 3px 2px; }
QTreeView::item:hover, QListWidget::item:hover { background: %bgLineHighlight%; }
QTreeView::item:selected, QListWidget::item:selected { background: %selection%; color: #ffffff; }
QLineEdit, QSpinBox, QComboBox, QFontComboBox { background: %bgEditor%; border: 1px solid %border%; border-radius: 4px; padding: 5px 8px; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid %accent%; }
QComboBox QAbstractItemView { background: %bgSideBar%; border: 1px solid %border%; selection-background-color: %accent%; }
QPushButton { background: %bgLineHighlight%; border: 1px solid %border%; border-radius: 5px; padding: 7px 16px; }
QPushButton:hover { border-color: %accent%; }
QPushButton:default, QPushButton#primary { background: %accent%; border-color: %accent%; color: #ffffff; }
QPushButton#primary:hover, QPushButton:default:hover { background: %accentLight%; }
QPushButton#link { background: transparent; border: none; color: %accentLight%; padding: 4px 0; text-align: left; }
QPushButton#link:hover { text-decoration: underline; }
QToolButton { background: transparent; border: none; border-radius: 4px; padding: 4px; }
QToolButton:hover { background: %border%; }
QToolButton#activity { border-radius: 0; border-left: 2px solid transparent; padding: 11px 0; }
QToolButton#activity:checked { border-left: 2px solid %accent%; background: %bgSideBar%; }
QToolButton#activity:hover { background: %bgSideBar%; }
QScrollBar:vertical { background: transparent; width: 11px; margin: 0; }
QScrollBar::handle:vertical { background: #2c2c34; border-radius: 5px; min-height: 28px; margin: 1px; }
QScrollBar::handle:vertical:hover { background: #3a3a44; }
QScrollBar:horizontal { background: transparent; height: 11px; margin: 0; }
QScrollBar::handle:horizontal { background: #2c2c34; border-radius: 5px; min-width: 28px; margin: 1px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QCheckBox::indicator { width: 15px; height: 15px; border: 1px solid #3a3a44; border-radius: 3px; background: %bgEditor%; }
QCheckBox::indicator:checked { background: %accent%; border-color: %accent%; }
QLabel#muted { color: %textMuted%; background: transparent; }
QLabel { background: transparent; }
QLabel#error { color: #f87171; }
)");
    const struct { const char *k; const QColor &c; } map[] = {
        {"%bgWindow%", bgWindow}, {"%bgActivity%", bgActivity}, {"%bgSideBar%", bgSideBar}, {"%bgEditor%", bgEditor},
        {"%bgLineHighlight%", bgLineHighlight}, {"%border%", border}, {"%textMuted%", textMuted},
        {"%accentLight%", accentLight}, {"%accent%", accent}, {"%selection%", selection}, {"%text%", text},
    };
    for (const auto &m : map)
        s.replace(QLatin1String(m.k), hex(m.c));
    return s;
}

void apply(QApplication &app)
{
    icons::ensureResources();
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette p;
    p.setColor(QPalette::Window, bgWindow);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, bgEditor);
    p.setColor(QPalette::AlternateBase, bgSideBar);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, bgLineHighlight);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase, bgSideBar);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, textMuted);
    p.setColor(QPalette::Disabled, QPalette::Text, QColor(0x55, 0x55, 0x5d));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x55, 0x55, 0x5d));
    app.setPalette(p);
    app.setStyleSheet(styleSheet());
}

} // namespace paw::theme
