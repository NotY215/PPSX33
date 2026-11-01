#include "MainWindow.h"
#include "NativeBridge.h"
#include "RenderWorkbench.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QShowEvent>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QFrame>
#include <QScrollArea>
#include <QList>
#include <QEasingCurve>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("PPSX33"));
    resize(1180, 760);
    setMinimumSize(960, 640);
    buildUi();
    applyTheme();
    if (!NativeBridge::instance().load()) {
        appendLog(QStringLiteral("[ui] ps3core: %1").arg(NativeBridge::instance().lastError()));
    } else {
        appendLog(QStringLiteral("[ui] ps3core v%1 loaded").arg(NativeBridge::instance().version()));
    }
    refreshButtons();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto* header = new QHBoxLayout;
    auto* brand = new QLabel(QStringLiteral("PPSX33"), this);
    brand->setObjectName(QStringLiteral("brand"));
    m_status = new QLabel(QStringLiteral("Ready"), this);
    m_status->setObjectName(QStringLiteral("status"));
    header->addWidget(brand);
    header->addStretch();
    header->addWidget(m_status);
    root->addLayout(header);

    auto* bar = new QHBoxLayout;
    bar->setSpacing(8);
    auto mk = [&](const QString& t) {
        auto* b = new QPushButton(t, this);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(36);
        bar->addWidget(b);
        return b;
    };
    m_btnLoad = mk(QStringLiteral("Load ELF"));
    m_btnLift = mk(QStringLiteral("Decompile"));
    m_btnBuild = mk(QStringLiteral("Build"));
    m_btnCopy = mk(QStringLiteral("Copy to Game Folder"));
    m_btnRun = mk(QStringLiteral("Run EXE"));
    m_btnCancel = mk(QStringLiteral("Stop / Cancel"));
    root->addLayout(bar);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 0);
    m_progress->setVisible(false);
    m_progress->setMaximumHeight(6);
    m_progress->setTextVisible(false);
    root->addWidget(m_progress);

    m_tabs = new QTabWidget(this);
    m_report = new QTextEdit(this);
    m_report->setReadOnly(true);
    m_report->setObjectName(QStringLiteral("report"));
    m_tabs->addTab(m_report, QStringLiteral("Report"));

    m_legal = new QTextEdit(this);
    m_legal->setReadOnly(true);
    m_legal->setObjectName(QStringLiteral("legal"));
    m_legal->setHtml(QStringLiteral(
        "<h2 style='color:#7dd3fc'>PPSX33 — Legal</h2>"
        "<p>Independent research recompiler. Not affiliated with Sony.</p>"
        "<ul><li>No bundled keys</li><li>Decrypted ELF only</li>"
        "<li>Use lawfully obtained materials only</li></ul>"
        "<p>Host branding is <b>PPSX33</b>.</p>"));
    m_tabs->addTab(m_legal, QStringLiteral("Legal"));

    m_controllerPage = new QWidget(this);
    auto* cl = new QVBoxLayout(m_controllerPage);
    cl->addWidget(new QLabel(QStringLiteral("Controller mapping is written to pad_map.json next to the game EXE after build."), this));
    cl->addStretch();
    m_tabs->addTab(m_controllerPage, QStringLiteral("Controller"));

    root->addWidget(m_tabs, 1);

    connect(m_btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadElf);
    connect(m_btnLift, &QPushButton::clicked, this, &MainWindow::onDecompile);
    connect(m_btnBuild, &QPushButton::clicked, this, &MainWindow::onBuild);
    connect(m_btnCopy, &QPushButton::clicked, this, &MainWindow::onCopy);
    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancel);

    m_fade = new QGraphicsOpacityEffect(central);
    central->setGraphicsEffect(m_fade);
    m_fade->setOpacity(0.0);
}

void MainWindow::applyTheme() {
    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background-color: #070b14; color: #e8eef7; font-family: 'Segoe UI', sans-serif; }"
        "QLabel#brand { font-size: 24px; font-weight: 800; color: #56c8ff; letter-spacing: 2px; }"
        "QLabel#status { color: #8ba3c7; font-size: 12px; }"
        "QPushButton {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #1a2740, stop:1 #121a2b);"
        "  border: 1px solid #2a3d5c; border-radius: 10px; padding: 9px 16px; color: #dbe7f8;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #243552, stop:1 #1a2740);"
        "  border-color: #56c8ff; color: #ffffff;"
        "}"
        "QPushButton:disabled { color: #5a6b84; background: #0d1320; border-color: #1a2233; }"
        "QPushButton:pressed { background: #56c8ff; color: #071018; border-color: #56c8ff; }"
        "QTabWidget::pane { border: 1px solid #1c2a40; border-radius: 12px; background: #0c1422; top: -1px; }"
        "QTabBar::tab { background: #0a101c; color: #7f95b5; padding: 11px 18px; margin-right: 3px;"
        "  border-top-left-radius: 10px; border-top-right-radius: 10px; border: 1px solid transparent; }"
        "QTabBar::tab:selected { background: #121c2e; color: #e8eef7; border-color: #1c2a40; border-bottom-color: #121c2e; }"
        "QTabBar::tab:hover { color: #56c8ff; }"
        "QTextEdit#report, QTextEdit#legal { background: #0a121f; border: none; padding: 10px;"
        "  font-family: Consolas, 'Cascadia Mono', monospace; font-size: 12px; color: #c5d4e8; selection-background-color: #1e3a5f; }"
        "QProgressBar { background: #121a2b; border: none; border-radius: 4px; max-height: 5px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #2b8cff, stop:1 #56c8ff); border-radius: 4px; }"
    ));
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    static bool once = false;
    if (!once) { once = true; playIntro();
        slideButtons(); }
}

void MainWindow::slideButtons() {
    QList<QPushButton*> btns = { m_btnLoad, m_btnLift, m_btnBuild, m_btnCopy, m_btnRun, m_btnCancel };
    int i = 0;
    for (QPushButton* b : btns) {
        if (!b) continue;
        auto* fx = new QGraphicsOpacityEffect(b);
        b->setGraphicsEffect(fx);
        fx->setOpacity(0.0);
        auto* a = new QPropertyAnimation(fx, "opacity", this);
        a->setEasingCurve(QEasingCurve::OutCubic);
        a->setStartValue(0.0);
        a->setEndValue(1.0);
        a->setDuration(420 + i * 75);
        a->start(QAbstractAnimation::DeleteWhenStopped);
        ++i;
    }
}

void MainWindow::playIntro() {
    auto* anim = new QPropertyAnimation(m_fade, "opacity", this);
    anim->setDuration(900);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::appendLog(const QString& s) {
    m_report->append(s);
    m_status->setText(s.left(80));
}

void MainWindow::setBusy(bool b) {
    m_busy = b;
    m_progress->setVisible(b);
    refreshButtons();
}

void MainWindow::refreshButtons() {
    m_btnLoad->setEnabled(!m_busy);
    m_btnLift->setEnabled(!m_busy && m_canLift);
    m_btnBuild->setEnabled(!m_busy && m_canBuild);
    m_btnCopy->setEnabled(!m_busy && m_canCopy);
    m_btnRun->setEnabled(!m_busy && m_canRun);
    m_btnCancel->setEnabled(m_busy);
    if (m_tabs) {
        int idx = m_tabs->indexOf(m_controllerPage);
        if (idx >= 0) m_tabs->setTabVisible(idx, !m_canBuild || m_canCopy);
    }
}

void MainWindow::onLoadElf() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load decrypted ELF"),
        QString(), QStringLiteral("ELF (*.elf *.bin);;All (*.*)"));
    if (path.isEmpty()) return;
    setBusy(true);
    QString root = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("projects"));
    QDir().mkpath(root);
    QFileInfo fi(path);
    QString game = fi.completeBaseName();
    QString pd, err;
    if (!NativeBridge::instance().createProject(root, game, path, &pd, &err)) {
        appendLog(QStringLiteral("[ui] create project failed: %1").arg(err));
        setBusy(false);
        return;
    }
    m_projectDir = pd;
    m_canLift = true;
    m_canBuild = m_canCopy = m_canRun = false;
    appendLog(QStringLiteral("[ui] project: %1").arg(m_projectDir));
    setBusy(false);
    refreshButtons();
}

void MainWindow::onDecompile() {
    if (m_projectDir.isEmpty()) return;
    setBusy(true);
    QString log;
    bool ok = NativeBridge::instance().liftProject(m_projectDir, &log);
    appendLog(log);
    if (ok) {
        m_canBuild = true;
        appendLog(QStringLiteral("[ui] decompile ok"));
    } else {
        appendLog(QStringLiteral("[ui] decompile failed"));
    }
    setBusy(false);
    refreshButtons();
}

void MainWindow::onBuild() {
    if (m_projectDir.isEmpty()) return;
    setBusy(true);
    NativeBridge::instance().beginBuild();
    QString dist = QCoreApplication::applicationDirPath();
    QString log;
    bool ok = NativeBridge::instance().buildProject(m_projectDir, dist, dist, &log);
    appendLog(log);
    if (ok) {
        m_canCopy = true;
        appendLog(QStringLiteral("[ui] build ok — Copy unlocked"));
    } else {
        appendLog(QStringLiteral("[ui] build failed"));
    }
    setBusy(false);
    refreshButtons();
}

void MainWindow::onCopy() {
    QString dest = QFileDialog::getExistingDirectory(this, QStringLiteral("Game USRDIR"));
    if (dest.isEmpty()) return;
    QString out = QDir(m_projectDir).filePath(QStringLiteral("output"));
    QDir outDir(out);
    if (!outDir.exists()) {
        appendLog(QStringLiteral("[ui] no output/"));
        return;
    }
    const QStringList dlls = {QStringLiteral("ps3rt.dll"), QStringLiteral("api.dll"),
        QStringLiteral("rsx.dll"), QStringLiteral("render.dll"), QStringLiteral("resolution.dll"),
        QStringLiteral("vulkan-1.dll"), QStringLiteral("ps3core.dll")};
    QString dist = QCoreApplication::applicationDirPath();
    for (const auto& d : dlls) {
        QString src = QDir(dist).filePath(d);
        if (QFile::exists(src))
            QFile::copy(src, QDir(dest).filePath(d));
    }
    QStringList exes = outDir.entryList(QStringList() << QStringLiteral("*.exe"), QDir::Files);
    for (const auto& e : exes) {
        QString target = QDir(dest).filePath(e);
        QFile::remove(target);
        QFile::copy(outDir.filePath(e), target);
        m_exePath = target;
    }
    m_gameRoot = dest;
    m_canRun = !m_exePath.isEmpty();
    appendLog(QStringLiteral("[ui] copied to %1").arg(dest));
    refreshButtons();
}

void MainWindow::onRun() {
    if (m_exePath.isEmpty()) return;
    QString logName = QStringLiteral("%1-%2.txt")
        .arg(QFileInfo(m_exePath).completeBaseName())
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss")));
    QString logPath = QDir(m_gameRoot).filePath(logName);
    QProcess::startDetached(m_exePath, {}, m_gameRoot);
    appendLog(QStringLiteral("[ui] launched %1 (log %2)").arg(m_exePath, logPath));
}

void MainWindow::onCancel() {
    NativeBridge::instance().cancelBuild();
    appendLog(QStringLiteral("[ui] cancel requested"));
    setBusy(false);
}
