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
        "QMainWindow, QWidget { background-color: #0b1220; color: #e2e8f0; font-family: 'Segoe UI'; }"
        "QLabel#brand { font-size: 22px; font-weight: 700; color: #38bdf8; letter-spacing: 1px; }"
        "QLabel#status { color: #94a3b8; }"
        "QPushButton { background: #1e293b; border: 1px solid #334155; border-radius: 8px; padding: 8px 14px; }"
        "QPushButton:hover { background: #334155; border-color: #38bdf8; }"
        "QPushButton:disabled { color: #64748b; background: #111827; border-color: #1f2937; }"
        "QPushButton:pressed { background: #0ea5e9; color: #0b1220; }"
        "QTabWidget::pane { border: 1px solid #1e293b; border-radius: 10px; background: #0f172a; }"
        "QTabBar::tab { background: #111827; color: #94a3b8; padding: 10px 16px; margin-right: 4px; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
        "QTabBar::tab:selected { background: #1e293b; color: #e2e8f0; }"
        "QTextEdit#report, QTextEdit#legal { background: #0f172a; border: none; font-family: Consolas, 'Cascadia Mono', monospace; font-size: 12px; }"
        "QProgressBar { background: #1e293b; border: none; border-radius: 3px; }"
        "QProgressBar::chunk { background: #38bdf8; border-radius: 3px; }"
    ));
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    static bool once = false;
    if (!once) { once = true; playIntro(); }
}

void MainWindow::playIntro() {
    auto* anim = new QPropertyAnimation(m_fade, "opacity", this);
    anim->setDuration(700);
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
