#pragma once
#include "editor/CodeEditor.h"

#include <QWidget>

class QSplitter;

namespace paw {

class EditorPane;
class FindWidget;

// Arranges one or more EditorPanes (Ctrl+F / Ctrl+H find bar overlay and
// "Split Editor" live here). Files are opened in the focused pane.
class EditorArea : public QWidget {
    Q_OBJECT
public:
    explicit EditorArea(QWidget *parent = nullptr);

    CodeEditor *currentEditor() const;
    CodeEditor *editorForPath(const QString &path) const;
    QList<CodeEditor *> editors() const;
    int count() const;
    int paneCount() const { return m_panes.size(); }

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

    // Split view
    void splitRight();                     // open a second pane with the current file
    void closeOtherPanes();                // back to a single pane (asks to save)
    bool isSplit() const { return m_panes.size() > 1; }

    // Find & Replace overlay (Ctrl+F / Ctrl+H shortcuts live in the area)
    void showFind(bool withReplace);

signals:
    void currentEditorChanged(CodeEditor *editor); // may be nullptr
    void editorCountChanged(int count);
    void fileOpened(const QString &path, bool ok);
    void message(const QString &text, int timeoutMs);
    void anyModifiedChanged();
    void editorStateChanged(); // cursor position, encoding, line ending or file name changed
    void paneCountChanged(int count);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    EditorPane *paneOf(CodeEditor *e) const;
    void setCurrentPane(EditorPane *p);
    void positionFindBar();

    QSplitter *m_split;
    QList<EditorPane *> m_panes;
    EditorPane *m_current = nullptr;
    FindWidget *m_find = nullptr;
    int m_untitledCounter = 0;
};

} // namespace paw
