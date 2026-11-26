#pragma once
#include <QPointer>
#include <QWidget>

class QCheckBox;
class QKeyEvent;
class QLabel;
class QLineEdit;
class QPushButton;

namespace paw {

class CodeEditor;

// Overlay bar with Find / Replace (Ctrl+F / Ctrl+H). Operates on the editor
// that is current in the EditorArea; the match highlights live in the editor.
class FindWidget : public QWidget {
    Q_OBJECT
public:
    explicit FindWidget(QWidget *parent = nullptr);

    void attach(CodeEditor *editor); // switch target editor, re-run the search
    void open(CodeEditor *editor, bool withReplace, const QString &initialText = QString());
    void closeBar();
    bool isVisibleBar() const { return isVisible(); }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void runSearch(bool keepCurrent);
    void findNext(bool backward);
    void replaceOne();
    void replaceAll();
    void updateMatches();

    QPointer<CodeEditor> m_editor;
    QLineEdit *m_find = nullptr;
    QLineEdit *m_replace = nullptr;
    QLabel *m_count = nullptr;
    QPushButton *m_caseBtn = nullptr;
    QWidget *m_replaceRow = nullptr;
    bool m_caseSensitive = false;
    int m_matches = 0;
};
