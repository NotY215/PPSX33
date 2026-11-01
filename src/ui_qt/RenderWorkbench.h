#pragma once
#include <QDialog>
class QTextEdit;
class QCheckBox;
class QPushButton;
class QLabel;
class QLineEdit;

class RenderWorkbench : public QDialog {
    Q_OBJECT
public:
    explicit RenderWorkbench(QWidget* parent = nullptr);
private slots:
    void onLoadElf();
    void onDecompile();
    void onAnalyze();
    void onGenerate();
private:
    void log(const QString& s);
    QString m_elf, m_project;
    bool m_lifted = false, m_analyzed = false;
    QTextEdit* m_console = nullptr;
    QLabel* m_target = nullptr;
    QCheckBox *m_cbApi = nullptr, *m_cbRsx = nullptr, *m_cbRender = nullptr;
    QCheckBox *m_cbRes = nullptr, *m_cbRt = nullptr;
};
