#include "EditorArea.h"

#include "editor/EditorPane.h"
#include "editor/FindWidget.h"

#include <QApplication>
#include <QLabel>
#include <QShortcut>
#include <QSplitter>
#include <QTextCursor>
#include <QVBoxLayout>

namespace paw {

EditorArea::EditorArea(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_split = new QSplitter(Qt::Horizontal, this);
    m_split->setChildrenCollapsible(false);
    layout->addWidget(m_split);

    auto *first = new EditorPane(this);
    m_panes.push_back(first);
    m_split->addWidget(first);
    m_current = first;

    // Forward every pane's signals; MainWindow only reads the current editor.
    for (EditorPane *p : std::as_const(m_panes)) {
        connect(p, &EditorPane::currentEditorChanged, this, [this](CodeEditor *e) {
            m_find->attach(e);
            emit currentEditorChanged(e);
        });
        connect(p, &EditorPane::editorCountChanged, this, [this](int) {
            int total = 0;
            for (EditorPane *pane : std::as_const(m_panes))
                total += pane->count();
            emit editorCountChanged(total);
        });
    }

    // Focus tracking: the pane that contains the focused editor becomes current.
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
        if (!now)
            return;
        for (EditorPane *p : std::as_const(m_panes)) {
            if (now == p || p->isAncestorOf(now)) {
                if (m_current != p) {
                    m_current = p;
                    emit currentEditorChanged(p->currentEditor());
                }
                break;
            }
        }
    });

    connect(first, &EditorPane::paneFocused, this, [this](EditorPane *p) { setCurrentPane(p); });
    connect(first, &EditorPane::fileOpened, this, &EditorArea::fileOpened);
    connect(first, &EditorPane::message, this, &EditorArea::message);
    connect(first, &EditorPane::anyModifiedChanged, this, &EditorArea::anyModifiedChanged);
    connect(first, &EditorPane::editorStateChanged, this, &EditorArea::editorStateChanged);

    // Find & Replace overlay
    m_find = new FindWidget(this);
    m_find->hide();
    auto *scFind = new QShortcut(QKeySequence::Find, this);
    scFind->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scFind, &QShortcut::activated, this, [this] { showFind(false); });
    auto *scReplace = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
    scReplace->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scReplace, &QShortcut::activated, this, [this] { showFind(true); });
}

void EditorArea::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    positionFindBar();
}

void EditorArea::positionFindBar()
{
    if (!m_find)
        return;
    m_find->adjustSize();
    m_find->move(qMax(8, width() - m_find->width() - 12), 6);
}

void EditorArea::showFind(bool withReplace)
{
    CodeEditor *e = currentEditor();
    if (!e)
        return;
    const QString sel = e->textCursor().selectedText();
    m_find->open(e, withReplace, sel);
    positionFindBar();
}

EditorPane *EditorArea::paneOf(CodeEditor *e) const
{
    if (!e)
        return nullptr;
    for (EditorPane *p : std::as_const(m_panes))
        if (p->editors().contains(e))
            return p;
    return nullptr;
}

void EditorArea::setCurrentPane(EditorPane *p)
{
    if (m_current == p)
        return;
    m_current = p;
    emit currentEditorChanged(p ? p->currentEditor() : nullptr);
}

CodeEditor *EditorArea::currentEditor() const
{
    return m_current ? m_current->currentEditor() : nullptr;
}

QList<CodeEditor *> EditorArea::editors() const
{
    QList<CodeEditor *> out;
    for (EditorPane *p : std::as_const(m_panes))
        out.append(p->editors());
    return out;
}

int EditorArea::count() const
{
    int total = 0;
    for (EditorPane *p : std::as_const(m_panes))
        total += p->count();
    return total;
}

CodeEditor *EditorArea::editorForPath(const QString &path) const
{
    for (EditorPane *p : std::as_const(m_panes))
        if (CodeEditor *e = p->editorForPath(path))
            return e;
    return nullptr;
}

void EditorArea::openFile(const QString &path)
{
    if (m_current)
        m_current->openFile(path);
}

void EditorArea::newUntitled()
{
    if (!m_current)
        return;
    ++m_untitledCounter;
    m_current->newUntitled(tr("Untitled-%1").arg(m_untitledCounter));
}

bool EditorArea::saveCurrent()
{
    return m_current ? m_current->saveCurrent() : true;
}

bool EditorArea::saveCurrentAs()
{
    return m_current ? m_current->saveCurrentAs() : true;
}

bool EditorArea::closeEditor(CodeEditor *e)
{
    EditorPane *p = paneOf(e);
    return p ? p->closeEditor(e) : true;
}

bool EditorArea::closeCurrent()
{
    return m_current ? m_current->closeCurrent() : true;
}

bool EditorArea::hasUnsavedChanges() const
{
    for (EditorPane *p : std::as_const(m_panes))
        if (p->hasUnsavedChanges())
            return true;
    return false;
}

bool EditorArea::saveAll()
{
    for (EditorPane *p : std::as_const(m_panes))
        if (!p->saveAll())
            return false;
    return true;
}

bool EditorArea::maybeSaveAll()
{
    for (EditorPane *p : std::as_const(m_panes))
        for (CodeEditor *e : p->editors())
            if (e->document()->isModified() && !p->maybeSave(e))
                return false;
    return true;
}

void EditorArea::closeAllWithoutAsking()
{
    for (EditorPane *p : std::as_const(m_panes))
        p->closeAllWithoutAsking();
    // back to a single pane
    while (m_panes.size() > 1) {
        EditorPane *p = m_panes.takeLast();
        m_split->removeWidget(p);
        p->deleteLater();
        emit paneCountChanged(1);
    }
    if (m_current)
        emit currentEditorChanged(m_current->currentEditor());
}

void EditorArea::closeEditorsUnder(const QString &path)
{
    for (EditorPane *p : std::as_const(m_panes))
        p->closeEditorsUnder(path);
}

void EditorArea::pathRenamed(const QString &oldPath, const QString &newPath)
{
    for (EditorPane *p : std::as_const(m_panes))
        p->pathRenamed(oldPath, newPath);
}

void EditorArea::applySettings()
{
    for (EditorPane *p : std::as_const(m_panes))
        p->applySettings();
}

void EditorArea::activate(CodeEditor *e)
{
    if (EditorPane *p = paneOf(e)) {
        setCurrentPane(p);
        p->activate(e);
    }
}

void EditorArea::splitRight()
{
    if (m_panes.size() >= 2)
        return;
    auto *pane = new EditorPane(this);
    m_panes.push_back(pane);
    m_split->addWidget(pane);
    m_split->setSizes({(width() / 2), (width() / 2)});

    connect(pane, &EditorPane::currentEditorChanged, this, [this](CodeEditor *e) {
        m_find->attach(e);
        if (sender() == m_current)
            emit currentEditorChanged(e);
    });
    connect(pane, &EditorPane::paneFocused, this, [this](EditorPane *p) { setCurrentPane(p); });
    connect(pane, &EditorPane::fileOpened, this, &EditorArea::fileOpened);
    connect(pane, &EditorPane::message, this, &EditorArea::message);
    connect(pane, &EditorPane::anyModifiedChanged, this, &EditorArea::anyModifiedChanged);
    connect(pane, &EditorPane::editorStateChanged, this, &EditorArea::editorStateChanged);
    connect(pane, &EditorPane::editorCountChanged, this, [this](int) { emit editorCountChanged(count()); });

    setCurrentPane(pane);
    if (CodeEditor *e = m_panes.first()->currentEditor()) {
        if (!e->isUntitled())
            pane->openFile(e->filePath());
        else
            pane->newUntitled(tr("Untitled"));
    }
    emit paneCountChanged(m_panes.size());
}

void EditorArea::closeOtherPanes()
{
    if (m_panes.size() <= 1)
        return;
    // Keep the current pane; ask to save the others.
    for (EditorPane *p : std::as_const(m_panes)) {
        if (p == m_current)
            continue;
        for (CodeEditor *e : p->editors()) {
            if (e->document()->isModified() && !p->closeEditor(e))
                return; // user cancelled
        }
    }
    while (m_panes.size() > 1) {
        EditorPane *p = m_panes.constLast();
        if (p == m_current) {
            p = m_panes.first();
        }
        m_panes.removeOne(p);
        m_split->removeWidget(p);
        p->deleteLater();
    }
    setCurrentPane(m_current);
    emit paneCountChanged(1);
    emit editorCountChanged(count());
}

} // namespace paw
