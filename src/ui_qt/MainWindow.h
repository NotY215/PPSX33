#pragma once
#include <QMainWindow>
#include <QMap>
class QLabel;
class QPushButton;
class QTextEdit;
class QTabWidget;
class QComboBox;
class QProgressBar;
class QLineEdit;
class QLineEdit;
class QTimer;
class UiLogoAnim;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
protected:
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
private slots:
    void onLoadElf();
    void onDecompile();
    void onBuild();
    void onCopy();
    void onRun();
    void onSaveKeys();
    void onGfxChanged(int);
    void onThreadsChanged(int);
        void onConsoleSubmit();
    void pollLiveBuild();
private:
    void buildUi();
    void applyTheme();
    void setBusy(bool b);
    void refreshButtons();
    void appendLog(const QString& s);
    void playIntro();
    void setTargetLabel(const QString& t);
    void buildControllerPage();
    void loadLegalHtml();
    void loadRequirementsHtml();
    void applySettingsToUi();

    QString m_projectDir, m_gameRoot, m_exePath;
    bool m_canLift = false, m_canBuild = false, m_canCopy = false, m_canRun = false, m_busy = false;

    QLabel *m_status = nullptr, *m_target = nullptr, *m_engine = nullptr;
    QLineEdit *m_consoleIn = nullptr;
    QTextEdit *m_console = nullptr, *m_report = nullptr, *m_roadmap = nullptr;
    QTextEdit *m_requirements = nullptr, *m_legal = nullptr;
    QWidget* m_controllerPage = nullptr;
    QTabWidget* m_tabs = nullptr;
    QComboBox *m_api = nullptr, *m_threads = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton *m_btnLoad = nullptr, *m_btnLift = nullptr, *m_btnBuild = nullptr;
    QPushButton *m_btnRun = nullptr, *m_btnCopy = nullptr, *m_btnSaveKeys = nullptr;
    QMap<QString, QLineEdit*> m_keyEdits;
    UiLogoAnim* m_logoAnim = nullptr;
};
