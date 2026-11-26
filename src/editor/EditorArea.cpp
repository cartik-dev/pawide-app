#include "EditorArea.h"

#include "ui/Icons.h"
#include "ui/Theme.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace paw {

static constexpr qint64 kMaxFileSize = 100LL * 1024 * 1024;

EditorArea::EditorArea(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack);

    // Page 0: watermark
    auto *wm = new QWidget(m_stack);
    auto *wl = new QVBoxLayout(wm);
    wl->setAlignment(Qt::AlignCenter);
    auto *logo = new QLabel(wm);
    logo->setPixmap(icons::brandPixmap(QStringLiteral(":/branding/paw-logo.svg"), 120, devicePixelRatioF()));
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(QStringLiteral("QLabel { background: transparent; }"));
    auto *hint = new QLabel(tr("Open a file from the Explorer to start editing"), wm);
    hint->setObjectName(QStringLiteral("muted"));
    hint->setAlignment(Qt::AlignCenter);
    wl->addWidget(logo);
    wl->addSpacing(12);
    wl->addWidget(hint);
    wm->setStyleSheet(QStringLiteral("QWidget { background: %1; }").arg(theme::bgEditor.name()));
    m_stack->addWidget(wm);

    // Page 1: tabs
    m_tabs = new QTabWidget(m_stack);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setDocumentMode(true);
    m_tabs->setUsesScrollButtons(true);
    m_stack->addWidget(m_tabs);

    connect(m_tabs, &QTabWidget::tabCloseRequested, this, [this](int i) {
        if (auto *e = qobject_cast<CodeEditor *>(m_tabs->widget(i)))
            closeEditor(e);
    });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) { emit currentEditorChanged(currentEditor()); });
    updateStack();
}

CodeEditor *EditorArea::currentEditor() const
{
    return qobject_cast<CodeEditor *>(m_tabs->currentWidget());
}

QList<CodeEditor *> EditorArea::editors() const
{
    QList<CodeEditor *> out;
    for (int i = 0; i < m_tabs->count(); ++i)
        if (auto *e = qobject_cast<CodeEditor *>(m_tabs->widget(i)))
            out.push_back(e);
    return out;
}

int EditorArea::count() const { return m_tabs->count(); }

CodeEditor *EditorArea::editorForPath(const QString &path) const
{
    const QString canon = QFileInfo(path).canonicalFilePath();
    for (CodeEditor *e : editors()) {
        if (!e->filePath().isEmpty() && (e->filePath() == path || QFileInfo(e->filePath()).canonicalFilePath() == canon))
            return e;
    }
    return nullptr;
}

void EditorArea::updateStack()
{
    m_stack->setCurrentIndex(m_tabs->count() == 0 ? 0 : 1);
}

CodeEditor *EditorArea::addEditor(CodeEditor *e)
{
    const int idx = m_tabs->addTab(e, e->displayName());
    connect(e->document(), &QTextDocument::modificationChanged, this, [this, e](bool) {
        refreshTab(e);
        emit anyModifiedChanged();
    });
    connect(e, &CodeEditor::fileInfoChanged, this, [this, e] {
        refreshTab(e);
        emit editorStateChanged();
    });
    connect(e, &CodeEditor::cursorInfoChanged, this, [this](int, int) { emit editorStateChanged(); });
    m_tabs->setCurrentIndex(idx);
    refreshTab(e);
    updateStack();
    emit editorCountChanged(m_tabs->count());
    emit currentEditorChanged(currentEditor());
    e->setFocus();
    return e;
}

void EditorArea::refreshTab(CodeEditor *e)
{
    const int i = m_tabs->indexOf(e);
    if (i < 0)
        return;
    m_tabs->setTabText(i, e->displayName() + (e->document()->isModified() ? QStringLiteral(" \u25CF") : QString()));
    m_tabs->setTabToolTip(i, e->filePath().isEmpty() ? e->displayName() : QDir::toNativeSeparators(e->filePath()));
}

void EditorArea::activate(CodeEditor *e)
{
    const int i = m_tabs->indexOf(e);
    if (i >= 0)
        m_tabs->setCurrentIndex(i);
}

void EditorArea::openFile(const QString &path)
{
    if (CodeEditor *existing = editorForPath(path)) {
        activate(existing);
        emit fileOpened(path, true);
        return;
    }
    if (m_loading.contains(path))
        return;
    const QFileInfo fi(path);
    if (!fi.exists() || !fi.isFile()) {
        QMessageBox::warning(this, tr("Open File"), tr("The file '%1' does not exist.").arg(fi.fileName()));
        emit fileOpened(path, false);
        return;
    }
    if (fi.size() > kMaxFileSize) {
        QMessageBox::warning(this, tr("Open File"),
                             tr("'%1' is larger than %2 MB and cannot be opened in the editor.")
                                 .arg(fi.fileName())
                                 .arg(kMaxFileSize / (1024 * 1024)));
        emit fileOpened(path, false);
        return;
    }
    m_loading.insert(path);
    emit message(tr("Loading %1…").arg(fi.fileName()), 0);
    auto *watcher = new QFutureWatcher<TextFile>(this);
    connect(watcher, &QFutureWatcher<TextFile>::finished, this, [this, watcher, path] {
        const TextFile tf = watcher->result();
        watcher->deleteLater();
        m_loading.remove(path);
        emit message(QString(), 1);
        if (!tf.ok) {
            QMessageBox::warning(this, tr("Open File"), tf.error.isEmpty() ? tr("Cannot open the file.") : tf.error);
            emit fileOpened(path, false);
            return;
        }
        if (editorForPath(path)) { // opened meanwhile
            emit fileOpened(path, true);
            return;
        }
        auto *e = new CodeEditor(this);
        e->setFilePath(path);
        e->loadFrom(tf);
        addEditor(e);
        emit fileOpened(path, true);
    });
    watcher->setFuture(QtConcurrent::run([path] { return FileUtils::readText(path); }));
}

void EditorArea::newUntitled()
{
    auto *e = new CodeEditor(this);
    ++m_untitledCounter;
    addEditor(e);
    const int i = m_tabs->indexOf(e);
    m_tabs->setTabText(i, tr("Untitled-%1").arg(m_untitledCounter));
}

bool EditorArea::saveCurrent()
{
    CodeEditor *e = currentEditor();
    if (!e)
        return true;
    if (e->isUntitled())
        return saveCurrentAs();
    QString err;
    if (!e->save(&err)) {
        QMessageBox::critical(this, tr("Save"), err);
        return false;
    }
    emit message(tr("Saved %1").arg(e->displayName()), 3000);
    return true;
}

bool EditorArea::saveCurrentAs()
{
    CodeEditor *e = currentEditor();
    if (!e)
        return true;
    const QString start = e->filePath();
    const QString path = QFileDialog::getSaveFileName(this, tr("Save As"), start);
    if (path.isEmpty())
        return false;
    QString err;
    if (!e->saveAs(path, &err)) {
        QMessageBox::critical(this, tr("Save As"), err);
        return false;
    }
    return true;
}

bool EditorArea::askAndSave(CodeEditor *e)
{
    activate(e);
    const QString name = e->displayName();
    const auto r = QMessageBox::warning(this, tr("Unsaved Changes"),
                                        tr("Do you want to save the changes you made to '%1'?\n\nYour changes will be lost if you don't save them.").arg(name),
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (r == QMessageBox::Cancel)
        return false;
    if (r == QMessageBox::Save)
        return saveCurrent();
    return true;
}

bool EditorArea::closeEditor(CodeEditor *e)
{
    if (e->document()->isModified() && !askAndSave(e))
        return false;
    const int i = m_tabs->indexOf(e);
    if (i >= 0) {
        m_tabs->removeTab(i);
        e->deleteLater();
    }
    updateStack();
    emit editorCountChanged(m_tabs->count());
    emit currentEditorChanged(currentEditor());
    emit anyModifiedChanged();
    return true;
}

bool EditorArea::closeCurrent()
{
    CodeEditor *e = currentEditor();
    return e ? closeEditor(e) : true;
}

bool EditorArea::hasUnsavedChanges() const
{
    for (CodeEditor *e : editors())
        if (e->document()->isModified())
            return true;
    return false;
}

bool EditorArea::saveAll()
{
    for (CodeEditor *e : editors()) {
        if (!e->document()->isModified())
            continue;
        activate(e);
        if (!saveCurrent())
            return false;
    }
    return true;
}

bool EditorArea::maybeSaveAll()
{
    for (CodeEditor *e : editors()) {
        if (e->document()->isModified() && !askAndSave(e))
            return false;
    }
    return true;
}

void EditorArea::closeAllWithoutAsking()
{
    for (CodeEditor *e : editors()) {
        const int i = m_tabs->indexOf(e);
        m_tabs->removeTab(i);
        e->deleteLater();
    }
    updateStack();
    emit editorCountChanged(0);
    emit currentEditorChanged(nullptr);
    emit anyModifiedChanged();
}

void EditorArea::closeEditorsUnder(const QString &path)
{
    const QString prefix = path + QLatin1Char('/');
    for (CodeEditor *e : editors()) {
        const QString p = e->filePath();
        if (p == path || p.startsWith(prefix)) {
            e->document()->setModified(false); // the file is gone; nothing to save to
            closeEditor(e);
        }
    }
}

void EditorArea::pathRenamed(const QString &oldPath, const QString &newPath)
{
    const QString prefix = oldPath + QLatin1Char('/');
    for (CodeEditor *e : editors()) {
        const QString p = e->filePath();
        if (p == oldPath)
            e->setFilePath(newPath);
        else if (p.startsWith(prefix))
            e->setFilePath(newPath + QLatin1Char('/') + p.mid(prefix.size()));
    }
}

void EditorArea::applySettings()
{
    for (CodeEditor *e : editors())
        e->applySettings();
}

} // namespace paw
