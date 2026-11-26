#pragma once
#include "editor/CodeEditor.h"

#include <QSet>
#include <QWidget>

class QStackedWidget;
class QTabWidget;

namespace paw {

// One tabbed pane of editors with an empty-state watermark. Files are loaded
// off the GUI thread. The EditorArea arranges one or more panes in a splitter.
class EditorPane : public QWidget {
    Q_OBJECT
public:
    explicit EditorPane(QWidget *parent = nullptr);

    CodeEditor *currentEditor() const;
    CodeEditor *editorForPath(const QString &path) const;
    QList<CodeEditor *> editors() const;
    int count() const;

    void openFile(const QString &path);
    void newUntitled(const QString &name);
    bool saveCurrent();
    bool saveCurrentAs();
    bool saveAll();
    bool closeCurrent();
    bool closeEditor(CodeEditor *e);       // asks to save; false if the user cancelled
    bool maybeSave(CodeEditor *e);         // asks to save a modified editor; false if cancelled
    bool hasUnsavedChanges() const;
    void closeAllWithoutAsking();
    void closeEditorsUnder(const QString &path); // after a file/folder was deleted
    void pathRenamed(const QString &oldPath, const QString &newPath);
    void applySettings();
    void activate(CodeEditor *e);

signals:
    void paneFocused(EditorPane *pane);    // a widget inside the pane gained keyboard focus
    void currentEditorChanged(CodeEditor *editor); // may be nullptr
    void editorCountChanged(int count);
    void fileOpened(const QString &path, bool ok);
    void message(const QString &text, int timeoutMs);
    void anyModifiedChanged();
    void editorStateChanged(); // cursor position, encoding, line ending or file name changed

private:
    CodeEditor *addEditor(CodeEditor *e);
    void refreshTab(CodeEditor *e);
    void updateStack();
    bool askAndSave(CodeEditor *e);

    QTabWidget *m_tabs;
    QStackedWidget *m_stack;
    QSet<QString> m_loading;
};

} // namespace paw
