#pragma once
#include <QDialog>

namespace paw {
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
};
} // namespace paw
