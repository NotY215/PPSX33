#pragma once
#include <QDialog>
class QTextEdit;
class QCheckBox;

class RenderWorkbench : public QDialog {
    Q_OBJECT
public:
    explicit RenderWorkbench(QWidget* parent = nullptr);
private:
    void playWelcome();
    QTextEdit* m_console = nullptr;
};
