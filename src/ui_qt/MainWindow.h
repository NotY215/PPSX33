#pragma once
#include <QMainWindow>
#include <QGraphicsOpacityEffect>
class QStackedWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QTabWidget;
class QProgressBar;
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

    QString m_projectDir;
    QString m_gameRoot;
    QString m_exePath;
    bool m_canLift = false, m_canBuild = false, m_canCopy = false, m_canRun = false;
    bool m_busy = false;

    QLabel* m_status = nullptr;
    QTextEdit* m_report = nullptr;
    QTextEdit* m_legal = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton *m_btnLoad = nullptr, *m_btnLift = nullptr, *m_btnBuild = nullptr;
    QPushButton *m_btnCopy = nullptr, *m_btnRun = nullptr, *m_btnCancel = nullptr;
    QTabWidget* m_tabs = nullptr;
    QWidget* m_controllerPage = nullptr;
    QGraphicsOpacityEffect* m_fade = nullptr;
};
