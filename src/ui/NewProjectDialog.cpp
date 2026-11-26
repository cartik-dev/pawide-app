#include "NewProjectDialog.h"

#include "core/AppContext.h"
#include "database/Database.h"
#include "project/ProjectTemplates.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace paw {

NewProjectDialog::NewProjectDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("New Project"));
    setMinimumWidth(520);
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(tr("MyProject"));
    form->addRow(tr("Project name"), m_name);

    auto *locRow = new QHBoxLayout;
    m_location = new QLineEdit(this);
    const QString last = appDatabase().value(QStringLiteral("newproject.location")).toString();
    m_location->setText(QDir::toNativeSeparators(
        last.isEmpty() || !QFileInfo(last).isDir() ? QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) : last));
    auto *browseBtn = new QPushButton(tr("Browse…"), this);
    locRow->addWidget(m_location, 1);
    locRow->addWidget(browseBtn);
    form->addRow(tr("Location"), locRow);
    root->addLayout(form);

    root->addWidget(new QLabel(tr("Template"), this));
    m_templates = new QListWidget(this);
    m_templates->setMinimumHeight(190);
    for (const TemplateInfo &t : ProjectTemplates::all()) {
        auto *item = new QListWidgetItem(QStringLiteral("%1\n%2").arg(t.name, t.description), m_templates);
        item->setData(Qt::UserRole, t.id);
    }
    m_templates->setCurrentRow(0);
    root->addWidget(m_templates);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("error"));
    m_status->setWordWrap(true);
    root->addWidget(m_status);

    auto *buttons = new QDialogButtonBox(this);
    m_createBtn = buttons->addButton(tr("Create Project"), QDialogButtonBox::AcceptRole);
    m_createBtn->setObjectName(QStringLiteral("primary"));
    buttons->addButton(QDialogButtonBox::Cancel);
    root->addWidget(buttons);

    connect(browseBtn, &QPushButton::clicked, this, &NewProjectDialog::browse);
    connect(m_name, &QLineEdit::textChanged, this, &NewProjectDialog::validate);
    connect(m_location, &QLineEdit::textChanged, this, &NewProjectDialog::validate);
    connect(buttons, &QDialogButtonBox::accepted, this, &NewProjectDialog::create);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    validate();
}

void NewProjectDialog::browse()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Location"), m_location->text());
    if (!dir.isEmpty())
        m_location->setText(QDir::toNativeSeparators(dir));
}

void NewProjectDialog::validate()
{
    QString err;
    const QString name = m_name->text();
    bool ok = !name.isEmpty();
    if (ok && !ProjectTemplates::validateProjectName(name, &err))
        ok = false;
    else if (ok && !QFileInfo(m_location->text()).isDir()) {
        err = tr("The selected location does not exist.");
        ok = false;
    }
    m_status->setText(name.isEmpty() ? QString() : err);
    m_createBtn->setEnabled(ok);
}

void NewProjectDialog::create()
{
    auto *item = m_templates->currentItem();
    if (!item)
        return;
    const CreateResult r = ProjectTemplates::create(item->data(Qt::UserRole).toString(), m_name->text(), m_location->text());
    if (!r.ok) {
        m_status->setText(r.error);
        return;
    }
    appDatabase().setValue(QStringLiteral("newproject.location"), QDir::fromNativeSeparators(m_location->text()));
    m_projectPath = r.projectPath;
    m_mainFile = r.mainFile;
    accept();
}

} // namespace paw
