#pragma once
#include <QMainWindow>
class QLabel;
class QPushButton;
class QTextEdit;
class QTabWidget;
class QComboBox;
class QProgressBar;
class QFrame;
class QPropertyAnimation;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
protected:
    void showEvent(QShowEvent* e) override;
private slots:
    void onLoadElf();
    void onDecompile();
    void onBuild();
    void onCopy();
    void onRun();
    void onCancel();
private:
    void buildUi();
    void applyTheme();
    void setBusy(bool b);
    void refreshButtons();
    void appendLog(const QString& s);
    void playIntro();
    void pulseLogo();
    void setTargetLabel(const QString& t);

    QString m_projectDir, m_gameRoot, m_exePath;
    bool m_canLift = false, m_canBuild = false, m_canCopy = false, m_canRun = false, m_busy = false;

    QLabel *m_logo = nullptr, *m_status = nullptr, *m_target = nullptr, *m_engine = nullptr;
    QTextEdit *m_console = nullptr, *m_report = nullptr, *m_roadmap = nullptr, *m_requirements = nullptr, *m_legal = nullptr;
    QWidget* m_controllerPage = nullptr;
    QTabWidget* m_tabs = nullptr;
    QComboBox *m_api = nullptr, *m_threads = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton *m_btnLoad = nullptr, *m_btnLift = nullptr, *m_btnBuild = nullptr;
    QPushButton *m_btnRun = nullptr, *m_btnCopy = nullptr, *m_btnCancel = nullptr;
};
