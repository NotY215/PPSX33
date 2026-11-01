#include "MainWindow.h"
#include "NativeBridge.h"
#include "Settings.h"
#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTabWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QFrame>
#include <QShowEvent>
#include <QCloseEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QSizePolicy>
#include <QPixmap>
#include <QIcon>
#include <QTimer>
#include <QList>
#include <QLineEdit>
#include <QScrollArea>
#include <QThread>
#include <QSysInfo>
#include <QtMath>

static QPushButton* makeSideBtn(const QString& text, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setCursor(Qt::PointingHandCursor);
    b->setMinimumHeight(38);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return b;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("PPSX33 · PlayStation 3 Recompiler"));
    resize(1280, 820);
    setMinimumSize(1024, 700);
    QString png = QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ppsx33-logo.png");
    QIcon icon(png);
    if (icon.isNull()) icon = QIcon(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (!icon.isNull()) setWindowIcon(icon);

    Settings::instance().load();
    buildUi();
    applyTheme();
    applySettingsToUi();

    if (!NativeBridge::instance().load())
        appendLog(QStringLiteral("ps3core load failed: %1").arg(NativeBridge::instance().lastError()));
    else
        appendLog(QStringLiteral("PPSX33 ready · core v%1").arg(NativeBridge::instance().version()));
    appendLog(QStringLiteral("Settings: %1").arg(Settings::instance().path()));
    appendLog(QStringLiteral("Host cores detected: %1").arg(QThread::idealThreadCount()));
    refreshButtons();
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* side = new QFrame(this);
    side->setObjectName(QStringLiteral("sidebar"));
    side->setFixedWidth(268);
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
    QPixmap pm(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ppsx33-logo.png"));
    if (pm.isNull()) pm = QPixmap(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (!pm.isNull())
        m_logo->setPixmap(pm.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

    auto* wf = new QLabel(QStringLiteral("WORKFLOW  ·  5 ACTIONS"), side);
    wf->setObjectName(QStringLiteral("section"));
    sideLay->addWidget(wf);

    m_btnLoad = makeSideBtn(QStringLiteral("Load ELF"), side);
    m_btnLift = makeSideBtn(QStringLiteral("Decompile"), side);
    m_btnBuild = makeSideBtn(QStringLiteral("Build Project"), side);
    m_btnCopy = makeSideBtn(QStringLiteral("Copy to Game Folder"), side);
    m_btnRun = makeSideBtn(QStringLiteral("Run Executable"), side);
    for (auto* b : {m_btnLoad, m_btnLift, m_btnBuild, m_btnCopy, m_btnRun})
        sideLay->addWidget(b);

    auto* setH = new QLabel(QStringLiteral("RECOMPILER SETTINGS"), side);
    setH->setObjectName(QStringLiteral("section"));
    sideLay->addSpacing(8);
    sideLay->addWidget(setH);

    auto* apiL = new QLabel(QStringLiteral("GRAPHICS API"), side);
    apiL->setObjectName(QStringLiteral("fieldLabel"));
    sideLay->addWidget(apiL);
    m_api = new QComboBox(side);
    m_api->addItem(QStringLiteral("Vulkan 1.2"), QStringLiteral("Vulkan"));
    m_api->addItem(QStringLiteral("OpenGL 4.3 (later)"), QStringLiteral("OpenGL"));
    sideLay->addWidget(m_api);

    auto* thrL = new QLabel(QStringLiteral("CPU THREADS"), side);
    thrL->setObjectName(QStringLiteral("fieldLabel"));
    sideLay->addWidget(thrL);
    m_threads = new QComboBox(side);
    const int cores = qMax(1, QThread::idealThreadCount());
    for (int i = 1; i <= cores; ++i)
        m_threads->addItem(QStringLiteral("%1").arg(i), i);
    sideLay->addWidget(m_threads);

    sideLay->addStretch();
    m_engine = new QLabel(QStringLiteral("ENGINE  READY"), side);
    m_engine->setObjectName(QStringLiteral("engineReady"));
    m_engine->setAlignment(Qt::AlignCenter);
    sideLay->addWidget(m_engine);
    auto* foot = new QLabel(QStringLiteral("Qt 6 · settings.json in AppData"), side);
    foot->setObjectName(QStringLiteral("foot"));
    foot->setWordWrap(true);
    sideLay->addWidget(foot);

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
    m_tabs->addTab(m_console, QStringLiteral("Console"));

    m_report = new QTextEdit(right);
    m_report->setReadOnly(true);
    m_tabs->addTab(m_report, QStringLiteral("Report"));

    m_roadmap = new QTextEdit(right);
    m_roadmap->setReadOnly(true);
    m_roadmap->setHtml(QStringLiteral(
        "<div style='font-family:Segoe UI;color:#c5d4e8;padding:10px'>"
        "<h2 style='color:#56c8ff'>Roadmap</h2>"
        "<ul>"
        "<li>Qt 6 host UI + AppData settings</li>"
        "<li>Modular runtime DLLs (ps3rt / api / rsx / render / resolution)</li>"
        "<li>Vulkan 1.2 present + FIFO drain path</li>"
        "<li>HLE progress + import recovery</li>"
        "<li>Title-driven GCM streams</li>"
        "</ul></div>"));
    m_tabs->addTab(m_roadmap, QStringLiteral("Roadmap"));

    m_requirements = new QTextEdit(right);
    m_requirements->setReadOnly(true);
    loadRequirementsHtml();
    m_tabs->addTab(m_requirements, QStringLiteral("Requirements"));

    buildControllerPage();
    m_tabs->addTab(m_controllerPage, QStringLiteral("Controller"));

    m_legal = new QTextEdit(right);
    m_legal->setReadOnly(true);
    loadLegalHtml();
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
    connect(m_api, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onGfxChanged);
    connect(m_threads, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onThreadsChanged);

    m_logoTimer = new QTimer(this);
    m_logoTimer->setInterval(16);
    connect(m_logoTimer, &QTimer::timeout, this, &MainWindow::onLogoTick);
}

void MainWindow::buildControllerPage() {
    m_controllerPage = new QWidget(this);
    auto* outer = new QVBoxLayout(m_controllerPage);
    auto* scroll = new QScrollArea(m_controllerPage);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* inner = new QWidget;
    auto* grid = new QGridLayout(inner);
    grid->setContentsMargins(8, 8, 8, 8);
    const auto keys = Settings::instance().keyMap();
    int row = 0;
    for (auto it = keys.begin(); it != keys.end(); ++it, ++row) {
        auto* lab = new QLabel(it.key(), inner);
        lab->setStyleSheet(QStringLiteral("color:#8ba3c7;"));
        auto* ed = new QLineEdit(it.value(), inner);
        ed->setMaximumWidth(160);
        m_keyEdits[it.key()] = ed;
        grid->addWidget(lab, row, 0);
        grid->addWidget(ed, row, 1);
    }
    scroll->setWidget(inner);
    outer->addWidget(scroll, 1);
    m_btnSaveKeys = new QPushButton(QStringLiteral("Save key map to settings.json"), m_controllerPage);
    connect(m_btnSaveKeys, &QPushButton::clicked, this, &MainWindow::onSaveKeys);
    outer->addWidget(m_btnSaveKeys);
}

void MainWindow::loadLegalHtml() {
    QString md;
    const QStringList paths = {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../docs/legal/LEGAL.md")),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../docs/legal/LEGAL.md")),
        QStringLiteral("docs/legal/LEGAL.md"),
    };
    for (const auto& p : paths) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly)) { md = QString::fromUtf8(f.readAll()); break; }
    }
    if (md.isEmpty()) {
        m_legal->setHtml(QStringLiteral(
            "<div style='font-family:Segoe UI;color:#c5d4e8;padding:12px;line-height:1.5'>"
            "<h2 style='color:#56c8ff'>PPSX33 — Legal Notice</h2>"
            "<p><b>Last updated:</b> 2026-10-28</p>"
            "<p>Independent research recompiler / HLE toolkit — <b>not affiliated with Sony</b>.</p>"
            "<ul>"
            "<li>No bundled keys; optional external decrypt is user-supplied.</li>"
            "<li>Decrypted ELF only for the load path in current builds.</li>"
            "<li>Use only lawfully obtained materials; no redistribution of game data.</li>"
            "<li>Host branding is <b>PPSX33</b>.</li>"
            "</ul>"
            "<p style='color:#8ba3c7'>See LICENSE, CODE_OF_CONDUCT, SECURITY at the repository root.</p>"
            "</div>"));
        return;
    }
    md.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    QString html = QStringLiteral("<div style='font-family:Segoe UI;color:#c5d4e8;padding:12px;line-height:1.55'>");
    for (QString line : md.split('\n')) {
        if (line.startsWith(QStringLiteral("# ")))
            html += QStringLiteral("<h2 style='color:#56c8ff'>%1</h2>").arg(line.mid(2).toHtmlEscaped());
        else if (line.startsWith(QStringLiteral("## ")))
            html += QStringLiteral("<h3 style='color:#7dd3fc'>%1</h3>").arg(line.mid(3).toHtmlEscaped());
        else if (line.startsWith(QStringLiteral("- ")))
            html += QStringLiteral("<div style='margin-left:12px'>• %1</div>").arg(line.mid(2).toHtmlEscaped());
        else if (line.trimmed().isEmpty())
            html += QStringLiteral("<br/>");
        else
            html += QStringLiteral("<p>%1</p>").arg(line.toHtmlEscaped());
    }
    html += QStringLiteral("</div>");
    m_legal->setHtml(html);
}

void MainWindow::loadRequirementsHtml() {
    m_requirements->setHtml(QStringLiteral(
        "<div style='font-family:Segoe UI;color:#c5d4e8;padding:12px;line-height:1.55'>"
        "<h2 style='color:#56c8ff'>Game runtime requirements</h2>"
        "<p>Applies to <b>running</b> a generated title EXE — not to building the recompiler UI.</p>"
        "<ul>"
        "<li><b>OS:</b> Windows x64</li>"
        "<li><b>Graphics:</b> Vulkan 1.2 capable GPU and driver</li>"
        "<li><b>OpenGL 4.3:</b> reserved for a later host path (not selectable yet)</li>"
        "<li><b>Present:</b> light host path (720×576 class)</li>"
        "<li><b>Audio:</b> XAudio2-capable host</li>"
        "</ul>"
        "<h3 style='color:#7dd3fc'>Developer machine (optional)</h3>"
        "<ul>"
        "<li>Visual Studio C++ toolset</li>"
        "<li>CMake 3.20+</li>"
        "<li>Qt 6.8 Widgets (via aqt)</li>"
        "</ul>"
        "<p style='color:#8ba3c7'>Detected logical processors on this PC: <b>%1</b></p>"
        "</div>").arg(QThread::idealThreadCount()));
}

void MainWindow::applySettingsToUi() {
    auto& s = Settings::instance();
    const QString gfx = s.graphicsBackend();
    int gi = m_api->findData(gfx);
    if (gi < 0) gi = 0;
    m_api->setCurrentIndex(gi);
    int ti = m_threads->findData(s.ppuThreads());
    if (ti < 0) {
        const int cores = qMax(1, QThread::idealThreadCount());
        ti = m_threads->findData(qMin(s.ppuThreads(), cores));
        if (ti < 0) ti = m_threads->count() - 1;
    }
    if (ti >= 0) m_threads->setCurrentIndex(ti);
}

void MainWindow::applyTheme() {
    setStyleSheet(QStringLiteral(
        "QMainWindow { background:#060a12; }"
        "QFrame#sidebar { background:#0a101c; border-right:1px solid #152033; }"
        "QFrame#logoBox { background:#0d1524; border:1px solid #1a2a44; border-radius:14px; padding:10px; }"
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
        "QComboBox QAbstractItemView { background:#0f1828; color:#d5e3f7; selection-background-color:#1e3a5f; }"
        "QTabWidget::pane { border:1px solid #1a2a40; border-radius:10px; background:#080e18; }"
        "QTabBar::tab { background:#0a101c; color:#7f95b5; padding:10px 16px; margin-right:2px; border-top-left-radius:8px; border-top-right-radius:8px; }"
        "QTabBar::tab:selected { background:#121c2e; color:#e8eef7; }"
        "QTabBar::tab:hover { color:#56c8ff; }"
        "QTextEdit { background:#080e18; color:#c5d4e8; border:none; font-family:Consolas,'Cascadia Mono',monospace; font-size:12px; padding:8px; }"
        "QLineEdit { background:#0f1828; color:#e8eef7; border:1px solid #243652; border-radius:6px; padding:6px; }"
        "QProgressBar { background:#121a2b; border:none; }"
        "QProgressBar::chunk { background:#56c8ff; }"
        "QScrollArea { background:transparent; border:none; }"
    ));
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    static bool once = false;
    if (!once) { once = true; playIntro(); m_logoTimer->start(); }
}

void MainWindow::closeEvent(QCloseEvent* e) {
    Settings::instance().save();
    QMainWindow::closeEvent(e);
}

void MainWindow::playIntro() {
    auto* fx = new QGraphicsOpacityEffect(centralWidget());
    centralWidget()->setGraphicsEffect(fx);
    auto* a = new QPropertyAnimation(fx, "opacity", this);
    a->setDuration(700);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->setEasingCurve(QEasingCurve::OutCubic);
    a->start(QAbstractAnimation::DeleteWhenStopped);
    QList<QPushButton*> btns = { m_btnLoad, m_btnLift, m_btnBuild, m_btnCopy, m_btnRun };
    int i = 0;
    for (auto* b : btns) {
        auto* bf = new QGraphicsOpacityEffect(b);
        b->setGraphicsEffect(bf);
        auto* ba = new QPropertyAnimation(bf, "opacity", this);
        ba->setDuration(360 + i * 65);
        ba->setStartValue(0.0);
        ba->setEndValue(1.0);
        ba->setEasingCurve(QEasingCurve::OutCubic);
        ba->start(QAbstractAnimation::DeleteWhenStopped);
        ++i;
    }
}

void MainWindow::onLogoTick() {
    if (!m_logo || !m_logo->pixmap() || m_logo->pixmap()->isNull()) return;
    m_logoPhase += 0.045;
    const qreal s = 1.0 + 0.035 * qSin(m_logoPhase);
    QPixmap base = QPixmap(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/ppsx33-logo.png"));
    if (base.isNull()) base = QPixmap(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (base.isNull()) return;
    const int side = int(100 * s);
    m_logo->setPixmap(base.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::setTargetLabel(const QString& t) { m_target->setText(t); }
void MainWindow::appendLog(const QString& s) { m_console->append(s); m_status->setText(s.left(120)); }
void MainWindow::setBusy(bool b) { m_busy = b; m_progress->setVisible(b); refreshButtons(); }

void MainWindow::refreshButtons() {
    m_btnLoad->setEnabled(!m_busy);
    m_btnLift->setEnabled(!m_busy && m_canLift);
    m_btnBuild->setEnabled(!m_busy && m_canBuild);
    m_btnCopy->setEnabled(!m_busy && m_canCopy);
    m_btnRun->setEnabled(!m_busy && m_canRun);
}

void MainWindow::onGfxChanged(int) {
    Settings::instance().setGraphicsBackend(m_api->currentData().toString());
    Settings::instance().save();
}

void MainWindow::onThreadsChanged(int) {
    Settings::instance().setPpuThreads(m_threads->currentData().toInt());
    Settings::instance().save();
}

void MainWindow::onSaveKeys() {
    for (auto it = m_keyEdits.begin(); it != m_keyEdits.end(); ++it)
        Settings::instance().setKey(it.key(), it.value()->text().trimmed());
    Settings::instance().save();
    appendLog(QStringLiteral("[ui] key map saved → %1").arg(Settings::instance().path()));
}

void MainWindow::onLoadElf() {
    QString start = Settings::instance().lastElfPath();
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load decrypted ELF"),
        start.isEmpty() ? QString() : QFileInfo(start).absolutePath(),
        QStringLiteral("ELF (*.elf *.bin);;All (*.*)"));
    if (path.isEmpty()) return;
    setBusy(true);
    Settings::instance().setLastElfPath(path);
    Settings::instance().save();
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
    if (!log.isEmpty()) { m_report->setPlainText(log); appendLog(log.left(400)); }
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
    QString start = Settings::instance().lastGameFolderPath();
    QString dest = QFileDialog::getExistingDirectory(this, QStringLiteral("Game USRDIR"), start);
    if (dest.isEmpty()) return;
    Settings::instance().setLastGameFolderPath(dest);
    Settings::instance().save();
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
