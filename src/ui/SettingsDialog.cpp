#include "SettingsDialog.h"

#include "core/AppInfo.h"
#include "settings/Settings.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace paw {

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    resize(640, 420);
    auto &s = Settings::instance();
    auto *root = new QHBoxLayout(this);
    auto *nav = new QListWidget(this);
    nav->setFixedWidth(160);
    auto *pages = new QStackedWidget(this);
    root->addWidget(nav);
    root->addWidget(pages, 1);

    // Editor
    auto *editor = new QWidget;
    auto *ef = new QFormLayout(editor);
    auto *tab = new QSpinBox(editor);
    tab->setRange(1, 16);
    tab->setValue(s.tabSize());
    connect(tab, &QSpinBox::valueChanged, this, [](int v) { Settings::instance().setTabSize(v); });
    auto *spaces = new QCheckBox(tr("Insert spaces when pressing Tab"), editor);
    spaces->setChecked(s.insertSpaces());
    connect(spaces, &QCheckBox::toggled, this, [](bool v) { Settings::instance().setInsertSpaces(v); });
    auto *wrap = new QCheckBox(tr("Word wrap"), editor);
    wrap->setChecked(s.wordWrap());
    connect(wrap, &QCheckBox::toggled, this, [](bool v) { Settings::instance().setWordWrap(v); });
    auto *fsize = new QSpinBox(editor);
    fsize->setRange(6, 48);
    fsize->setValue(s.fontSize());
    connect(fsize, &QSpinBox::valueChanged, this, [](int v) { Settings::instance().setFontSize(v); });
    auto *ffam = new QLineEdit(s.fontFamily(), editor);
    ffam->setPlaceholderText(tr("Default monospace font"));
    connect(ffam, &QLineEdit::editingFinished, this, [ffam] { Settings::instance().setFontFamily(ffam->text().trimmed()); });
    ef->addRow(tr("Tab size"), tab);
    ef->addRow(QString(), spaces);
    ef->addRow(QString(), wrap);
    ef->addRow(tr("Font size"), fsize);
    ef->addRow(tr("Font family"), ffam);
    pages->addWidget(editor);
    nav->addItem(tr("Editor"));

    // Explorer
    auto *explorer = new QWidget;
    auto *xl = new QVBoxLayout(explorer);
    auto *hidden = new QCheckBox(tr("Show hidden files"), explorer);
    hidden->setChecked(s.showHiddenFiles());
    connect(hidden, &QCheckBox::toggled, this, [](bool v) { Settings::instance().setShowHiddenFiles(v); });
    auto *confirm = new QCheckBox(tr("Confirm before deleting"), explorer);
    confirm->setChecked(s.confirmDelete());
    connect(confirm, &QCheckBox::toggled, this, [](bool v) { Settings::instance().setConfirmDelete(v); });
    xl->addWidget(hidden);
    xl->addWidget(confirm);
    xl->addStretch(1);
    pages->addWidget(explorer);
    nav->addItem(tr("Explorer"));

    // About
    auto *about = new QWidget;
    auto *al = new QVBoxLayout(about);
    al->addWidget(new QLabel(tr("<b>Paw IDE</b> %1").arg(appVersion()), about));
    al->addWidget(new QLabel(tr("Code without limits."), about));
    auto *repo = new QPushButton(repositoryUrl(), about);
    repo->setObjectName(QStringLiteral("link"));
    connect(repo, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl(repositoryUrl())); });
    al->addWidget(repo);
    al->addStretch(1);
    pages->addWidget(about);
    nav->addItem(tr("About"));

    connect(nav, &QListWidget::currentRowChanged, pages, &QStackedWidget::setCurrentIndex);
    nav->setCurrentRow(0);
}

} // namespace paw
