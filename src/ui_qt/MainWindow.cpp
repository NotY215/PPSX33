#include "MainWindow.h"
#include "NativeBridge.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileInfo>
#include <QSizePolicy>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QFrame>
#include <QShowEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QPixmap>
#include <QIcon>
#include <QTimer>
#include <QList>

static QPushButton* makeSideBtn(const QString& text, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumHeight(38);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return b;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("PPSX33 · PlayStation 3 Recompiler"));
    resize(1280, 800);
    setMinimumSize(1024, 680);
    QIcon icon(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (icon.isNull())
        icon = QIcon(QCoreApplication::applicationDirPath() + QStringLiteral("/../assets/ppsx33-logo.png"));
    if (!icon.isNull()) setWindowIcon(icon);
    buildUi();
    applyTheme();
    if (!NativeBridge::instance().load())
        appendLog(QStringLiteral("ps3core load failed: %1").arg(NativeBridge::instance().lastError()));
    else
        appendLog(QStringLiteral("PPSX33 ready · core v%1").arg(NativeBridge::instance().version()));
    appendLog(QStringLiteral("UI: Qt 6 Widgets · graphics default Vulkan 1.2"));
    refreshButtons();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ---- LEFT SIDEBAR ----
    auto* side = new QFrame(this);
    side->setObjectName(QStringLiteral("sidebar"));
    side->setFixedWidth(260);
    auto* sideLay = new QVBoxLayout(side);
    sideLay->setContentsMargins(16, 18, 16, 16);
    sideLay->setSpacing(10);

    auto* logoBox = new QFrame(side);
    logoBox->setObjectName(QStringLiteral("logoBox"));
    auto* logoLay = new QVBoxLayout(logoBox);
    logoLay->setAlignment(Qt::AlignCenter);
    m_logo = new QLabel(logoBox);
    m_logo->setObjectName(QStringLiteral("logo"));
    m_logo->setAlignment(Qt::AlignCenter);
    QPixmap pm(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (pm.isNull())
        pm = QPixmap(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ppsx33-logo.png"));
    if (!pm.isNull())
        m_logo->setPixmap(pm.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
        m_logo->setText(QStringLiteral("PPSX33"));
    auto* title = new QLabel(QStringLiteral("PPSX33"), logoBox);
    title->setObjectName(QStringLiteral("sideTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto* sub = new QLabel(QStringLiteral("PS3 BINARY RECOMPILER"), logoBox);
    sub->setObjectName(QStringLiteral("sideSub"));
    sub->setAlignment(Qt::AlignCenter);
    logoLay->addWidget(m_logo);
    logoLay->addWidget(title);
    logoLay->addWidget(sub);
    sideLay->addWidget(logoBox);

    auto* wf = new QLabel(QStringLiteral("WORKFLOW  ·  6 ACTIONS"), side);
    wf->setObjectName(QStringLiteral("section"));
    sideLay->addWidget(wf);

    m_btnLoad = makeSideBtn(QStringLiteral("Load ELF"), side);
    m_btnLift = makeSideBtn(QStringLiteral("Decompile"), side);
    m_btnBuild = makeSideBtn(QStringLiteral("Build Project"), side);
    m_btnRun = makeSideBtn(QStringLiteral("Run Executable"), side);
    m_btnCopy = makeSideBtn(QStringLiteral("Copy to Game Folder"), side);
    m_btnCancel = makeSideBtn(QStringLiteral("Stop Game"), side);
    for (auto* b : {m_btnLoad, m_btnLift, m_btnBuild, m_btnRun, m_btnCopy, m_btnCancel})
        sideLay->addWidget(b);

    auto* setH = new QLabel(QStringLiteral("RECOMPILER SETTINGS"), side);
    setH->setObjectName(QStringLiteral("section"));
    sideLay->addSpacing(8);
    sideLay->addWidget(setH);

    auto* apiL = new QLabel(QStringLiteral("GRAPHICS API"), side);
    apiL->setObjectName(QStringLiteral("fieldLabel"));
    sideLay->addWidget(apiL);
    m_api = new QComboBox(side);
    m_api->addItem(QStringLiteral("Vulkan 1.2"), 0);
    m_api->addItem(QStringLiteral("OpenGL 4.3 (later)"), 1);
    m_api->setCurrentIndex(0);
    sideLay->addWidget(m_api);

    auto* thrL = new QLabel(QStringLiteral("CPU THREADS"), side);
    thrL->setObjectName(QStringLiteral("fieldLabel"));
    sideLay->addWidget(thrL);
    m_threads = new QComboBox(side);
    for (int i = 1; i <= 16; ++i) m_threads->addItem(QString::number(i));
    m_threads->setCurrentIndex(3);
    sideLay->addWidget(m_threads);

    sideLay->addStretch();
    m_engine = new QLabel(QStringLiteral("ENGINE  READY"), side);
    m_engine->setObjectName(QStringLiteral("engineReady"));
    m_engine->setAlignment(Qt::AlignCenter);
    sideLay->addWidget(m_engine);
    auto* foot = new QLabel(QStringLiteral("Toolchain: system VS · Qt 6 UI"), side);
    foot->setObjectName(QStringLiteral("foot"));
    foot->setWordWrap(true);
    sideLay->addWidget(foot);

    // ---- RIGHT CONTENT ----
    auto* right = new QWidget(this);
    auto* rightLay = new QVBoxLayout(right);
    rightLay->setContentsMargins(14, 12, 14, 12);
    rightLay->setSpacing(10);

    m_target = new QLabel(QStringLiteral("TARGET  /  NO PROJECT LOADED"), right);
    m_target->setObjectName(QStringLiteral("targetBar"));
    m_target->setAlignment(Qt::AlignCenter);
    rightLay->addWidget(m_target);

    m_progress = new QProgressBar(right);
    m_progress->setRange(0, 0);
    m_progress->setVisible(false);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(4);
    rightLay->addWidget(m_progress);

    m_tabs = new QTabWidget(right);
    m_console = new QTextEdit(right);
    m_console->setReadOnly(true);
    m_console->setObjectName(QStringLiteral("console"));
    m_tabs->addTab(m_console, QStringLiteral("Console"));

    m_report = new QTextEdit(right);
    m_report->setReadOnly(true);
    m_tabs->addTab(m_report, QStringLiteral("Report"));

    m_roadmap = new QTextEdit(right);
    m_roadmap->setReadOnly(true);
    m_roadmap->setPlainText(QStringLiteral(
        "PPSX33 roadmap (host)\n"
        "- Qt 6 UI workflow\n"
        "- Modular runtime DLLs (ps3rt / api / rsx / render / resolution)\n"
        "- Vulkan 1.2 present + FIFO drain\n"
        "- HLE progress + import island recovery\n"
        "- Title-driven GCM streams"));
    m_tabs->addTab(m_roadmap, QStringLiteral("Roadmap"));

    m_requirements = new QTextEdit(right);
    m_requirements->setReadOnly(true);
    m_requirements->setPlainText(QStringLiteral(
        "Game runtime requirements\n"
        "- Windows x64\n"
        "- Vulkan 1.2 capable GPU/driver\n"
        "- OpenGL 4.3 reserved (not selectable yet)\n"
        "- Host present 720x576 light path\n"
        "\n"
        "Developer tools (not required to run games)\n"
        "- Visual Studio C++ toolchain\n"
        "- CMake 3.20+\n"
        "- Qt 6.8 Widgets (aqt install)"));
    m_tabs->addTab(m_requirements, QStringLiteral("Requirements"));

    m_controllerPage = new QWidget(right);
    auto* cl = new QVBoxLayout(m_controllerPage);
    cl->addWidget(new QLabel(QStringLiteral("Controller mapping is saved as pad_map.json next to the game EXE after Copy."), m_controllerPage));
    cl->addStretch();
    m_tabs->addTab(m_controllerPage, QStringLiteral("Controller"));

    m_legal = new QTextEdit(right);
    m_legal->setReadOnly(true);
    m_legal->setHtml(QStringLiteral(
        "<div style='font-family:Segoe UI;color:#c5d4e8;padding:8px'>"
        "<h2 style='color:#56c8ff;margin:0 0 8px 0'>PPSX33 — Legal Notice</h2>"
        "<p><b>Last updated:</b> 2026-10-28</p>"
        "<p>Independent research recompiler / HLE toolkit — <b>not affiliated with Sony</b>.</p>"
        "<ul>"
        "<li>No bundled keys; optional external decrypt is user-supplied.</li>"
        "<li>Decrypted ELF only for the load path in current builds.</li>"
        "<li>Use only lawfully obtained materials; no redistribution of game data.</li>"
        "<li>Host branding is <b>PPSX33</b>.</li>"
        "</ul>"
        "<p style='color:#8ba3c7'>See repository LICENSE / CODE_OF_CONDUCT / SECURITY at the project root.</p>"
        "</div>"));
    m_tabs->addTab(m_legal, QStringLiteral("Legal"));

    rightLay->addWidget(m_tabs, 1);
    m_status = new QLabel(QStringLiteral("Ready"), right);
    m_status->setObjectName(QStringLiteral("statusLine"));
    rightLay->addWidget(m_status);

    root->addWidget(side);
    root->addWidget(right, 1);

    connect(m_btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadElf);
    connect(m_btnLift, &QPushButton::clicked, this, &MainWindow::onDecompile);
    connect(m_btnBuild, &QPushButton::clicked, this, &MainWindow::onBuild);
    connect(m_btnCopy, &QPushButton::clicked, this, &MainWindow::onCopy);
    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(m_btnCancel, &QPushButton::clicked, this, &MainWindow::onCancel);
}

void MainWindow::applyTheme() {
    setStyleSheet(QStringLiteral(
        "QMainWindow { background:#060a12; }"
        "QFrame#sidebar { background:#0a101c; border-right:1px solid #152033; }"
        "QFrame#logoBox { background:#0d1524; border:1px solid #1a2a44; border-radius:12px; padding:8px; }"
        "QLabel#sideTitle { color:#56c8ff; font-size:18px; font-weight:800; letter-spacing:1px; }"
        "QLabel#sideSub { color:#6b83a6; font-size:10px; letter-spacing:1px; }"
        "QLabel#section { color:#5b7396; font-size:10px; font-weight:700; letter-spacing:1px; margin-top:6px; }"
        "QLabel#fieldLabel { color:#7f95b5; font-size:10px; font-weight:600; }"
        "QLabel#engineReady { background:#0c1f18; color:#3ddc97; border:1px solid #1a3d30; border-radius:8px; padding:10px; font-weight:700; letter-spacing:1px; }"
        "QLabel#foot { color:#4d617d; font-size:10px; }"
        "QLabel#targetBar { background:#0a1220; color:#56c8ff; border:1px solid #1a2f4d; border-radius:8px; padding:10px; font-weight:700; letter-spacing:1px; }"
        "QLabel#statusLine { color:#7f95b5; font-size:11px; }"
        "QPushButton { background:#0f1828; color:#d5e3f7; border:1px solid #243652; border-radius:10px; text-align:left; padding-left:14px; }"
        "QPushButton:hover { border-color:#56c8ff; background:#152238; }"
        "QPushButton:disabled { color:#4a5d78; background:#0a1018; border-color:#152033; }"
        "QPushButton:pressed { background:#56c8ff; color:#061018; }"
        "QComboBox { background:#0f1828; color:#d5e3f7; border:1px solid #243652; border-radius:8px; padding:6px 10px; }"
        "QComboBox::drop-down { border:none; }"
        "QComboBox QAbstractItemView { background:#0f1828; color:#d5e3f7; selection-background-color:#1e3a5f; }"
        "QTabWidget::pane { border:1px solid #1a2a40; border-radius:10px; background:#080e18; }"
        "QTabBar::tab { background:#0a101c; color:#7f95b5; padding:10px 16px; margin-right:2px; border-top-left-radius:8px; border-top-right-radius:8px; }"
        "QTabBar::tab:selected { background:#121c2e; color:#e8eef7; }"
        "QTabBar::tab:hover { color:#56c8ff; }"
        "QTextEdit { background:#080e18; color:#c5d4e8; border:none; font-family:Consolas,'Cascadia Mono',monospace; font-size:12px; padding:8px; }"
        "QProgressBar { background:#121a2b; border:none; border-radius:2px; }"
        "QProgressBar::chunk { background:#56c8ff; }"
    ));
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    static bool once = false;
    if (!once) { once = true; playIntro(); pulseLogo(); }
}

void MainWindow::playIntro() {
    auto* fx = new QGraphicsOpacityEffect(centralWidget());
    centralWidget()->setGraphicsEffect(fx);
    auto* a = new QPropertyAnimation(fx, "opacity", this);
    a->setDuration(850);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);

    QList<QPushButton*> btns = { m_btnLoad, m_btnLift, m_btnBuild, m_btnRun, m_btnCopy, m_btnCancel };
    int i = 0;
    for (auto* b : btns) {
        auto* bf = new QGraphicsOpacityEffect(b);
        b->setGraphicsEffect(bf);
        bf->setOpacity(0.0);
        auto* ba = new QPropertyAnimation(bf, "opacity", this);
        ba->setDuration(380 + i * 70);
        ba->setStartValue(0.0);
        ba->setEndValue(1.0);
        ba->setEasingCurve(QEasingCurve::OutCubic);
        ba->start(QAbstractAnimation::DeleteWhenStopped);
        ++i;
    }
}

void MainWindow::pulseLogo() {
    if (!m_logo) return;
    auto* a = new QPropertyAnimation(m_logo, "geometry", this);
    QRect r = m_logo->geometry();
    a->setDuration(1600);
    a->setLoopCount(-1);
    a->setKeyValueAt(0.0, r);
    a->setKeyValueAt(0.5, QRect(r.x(), r.y() - 3, r.width(), r.height()));
    a->setKeyValueAt(1.0, r);
    a->setEasingCurve(QEasingCurve::InOutSine);
    QTimer::singleShot(200, this, [a] { a->start(); });
}

void MainWindow::setTargetLabel(const QString& t) { m_target->setText(t); }

void MainWindow::appendLog(const QString& s) {
    m_console->append(s);
    m_status->setText(s.left(100));
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
    m_btnCancel->setEnabled(true);
}

void MainWindow::onLoadElf() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load decrypted ELF"),
        QString(), QStringLiteral("ELF (*.elf *.bin);;All (*.*)"));
    if (path.isEmpty()) return;
    setBusy(true);
    QString root = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("projects"));
    QDir().mkpath(root);
    QString game = QFileInfo(path).completeBaseName();
    QString pd, err;
    if (!NativeBridge::instance().createProject(root, game, path, &pd, &err)) {
        appendLog(QStringLiteral("[ui] create failed: %1").arg(err));
        setBusy(false);
        return;
    }
    m_projectDir = pd;
    m_canLift = true;
    m_canBuild = m_canCopy = m_canRun = false;
    setTargetLabel(QStringLiteral("TARGET  /  %1").arg(game.toUpper()));
    appendLog(QStringLiteral("[ui] project %1").arg(m_projectDir));
    setBusy(false);
}

void MainWindow::onDecompile() {
    if (m_projectDir.isEmpty()) return;
    setBusy(true);
    QString log;
    bool ok = NativeBridge::instance().liftProject(m_projectDir, &log);
    if (!log.isEmpty()) { m_report->setPlainText(log); appendLog(log.left(500)); }
    m_canBuild = ok;
    appendLog(ok ? QStringLiteral("[ui] decompile ok") : QStringLiteral("[ui] decompile failed"));
    setBusy(false);
}

void MainWindow::onBuild() {
    if (m_projectDir.isEmpty()) return;
    setBusy(true);
    NativeBridge::instance().beginBuild();
    QString dist = QCoreApplication::applicationDirPath();
    QString log;
    bool ok = NativeBridge::instance().buildProject(m_projectDir, dist, dist, &log);
    if (!log.isEmpty()) m_report->append(log);
    m_canCopy = ok;
    appendLog(ok ? QStringLiteral("[ui] build ok") : QStringLiteral("[ui] build failed"));
    setBusy(false);
}

void MainWindow::onCopy() {
    QString dest = QFileDialog::getExistingDirectory(this, QStringLiteral("Game USRDIR"));
    if (dest.isEmpty()) return;
    QString dist = QCoreApplication::applicationDirPath();
    for (const char* d : {"ps3rt.dll","api.dll","rsx.dll","render.dll","resolution.dll","vulkan-1.dll","ps3core.dll"}) {
        QString src = QDir(dist).filePath(QString::fromUtf8(d));
        if (QFile::exists(src)) QFile::copy(src, QDir(dest).filePath(QString::fromUtf8(d)));
    }
    QDir out(QDir(m_projectDir).filePath(QStringLiteral("output")));
    for (const auto& e : out.entryList(QStringList() << QStringLiteral("*.exe"), QDir::Files)) {
        QString t = QDir(dest).filePath(e);
        QFile::remove(t);
        QFile::copy(out.filePath(e), t);
        m_exePath = t;
    }
    m_gameRoot = dest;
    m_canRun = !m_exePath.isEmpty();
    appendLog(QStringLiteral("[ui] copied → %1").arg(dest));
    refreshButtons();
}

void MainWindow::onRun() {
    if (m_exePath.isEmpty()) return;
    QProcess::startDetached(m_exePath, {}, m_gameRoot);
    appendLog(QStringLiteral("[ui] launched %1").arg(m_exePath));
}

void MainWindow::onCancel() {
    NativeBridge::instance().cancelBuild();
    appendLog(QStringLiteral("[ui] stop / cancel"));
    setBusy(false);
}
