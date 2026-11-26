#pragma once
#include <QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace paw {

class NewProjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit NewProjectDialog(QWidget *parent = nullptr);

    QString projectPath() const { return m_projectPath; }
    QString mainFile() const { return m_mainFile; }

private:
    void browse();
    void validate();
    void create();

    QLineEdit *m_name;
    QLineEdit *m_location;
    QListWidget *m_templates;
    QLabel *m_status;
    QPushButton *m_createBtn;
    QString m_projectPath;
    QString m_mainFile;
};

} // namespace paw
