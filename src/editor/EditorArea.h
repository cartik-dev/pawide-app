#pragma once
#include "editor/CodeEditor.h"

#include <QSet>
#include <QWidget>

class QLabel;
class QStackedWidget;
class QTabWidget;

namespace paw {

// Tabbed editor area with an empty-state watermark. Files are loaded off the GUI thread.
class EditorArea : public QWidget {
    Q_OBJECT
public:
    explicit EditorArea(QWidget *parent = nullptr);

    CodeEditor *currentEditor() const;
    CodeEditor *editorForPath(const QString &path) const;
    QList<CodeEditor *> editors() const;
    int count() const;

    void openFile(const QString &path);
    void newUntitled();
    bool saveCurrent();
    bool saveCurrentAs();
    bool saveAll();
    bool closeCurrent();
    bool closeEditor(CodeEditor *e);       // asks to save; false if the user cancelled
    bool maybeSaveAll();                   // asks for every modified editor; false if cancelled
    bool hasUnsavedChanges() const;
    void closeAllWithoutAsking();
    void closeEditorsUnder(const QString &path); // after a file/folder was deleted
    void pathRenamed(const QString &oldPath, const QString &newPath);
    void applySettings();
    void activate(CodeEditor *e);

signals:
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
    int m_untitledCounter = 0;
};

} // namespace paw
