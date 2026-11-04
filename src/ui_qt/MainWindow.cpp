#include "MainWindow.h"
#include "AppVersion.h"
#include "NativeBridge.h"
#include "Settings.h"
#include "EmbeddedLegal.h"
#include "UiLogoAnim.h"
#include "JobWorker.h"
#include <QThread>
#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
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
#include <QElapsedTimer>
#include <QDateTime>
#include <QRegularExpression>
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
    setWindowTitle(QStringLiteral("PPSX33 · PlayStation 3 Recompiler · %1").arg(QStringLiteral("1.1.0-alpha")));
    resize(1280, 820);
    setMinimumSize(1024, 700);
    QString png = QCoreApplication::applicationDirPath() + QStringLiteral("/ppsx33-logo.png");
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
        appendLog(QStringLiteral("PPSX33 %1 · core v%2").arg(QStringLiteral(PPSX33_VERSION_STRING)).arg(NativeBridge::instance().version()));
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
    m_logoAnim = new UiLogoAnim(logoBox);
    m_logoAnim->setLogoPath(QCoreApplication::applicationDirPath() + QStringLiteral("/ppsx33-logo.png"));
    auto* title = new QLabel(QStringLiteral("PPSX33"), logoBox);
    title->setObjectName(QStringLiteral("sideTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto* sub = new QLabel(QStringLiteral("PS3 BINARY RECOMPILER"), logoBox);
    sub->setObjectName(QStringLiteral("sideSub"));
    sub->setAlignment(Qt::AlignCenter);
    logoLay->addWidget(m_logoAnim, 0, Qt::AlignCenter);
    logoLay->addWidget(title);
    logoLay->addWidget(sub);
    sideLay->addWidget(logoBox);

    auto* wf = new QLabel(QStringLiteral("WORKFLOW  ·  6 ACTIONS"), side);
    wf->setObjectName(QStringLiteral("section"));
    sideLay->addWidget(wf);

    m_btnLoad = makeSideBtn(QStringLiteral("Load ELF"), side);
    m_btnLift = makeSideBtn(QStringLiteral("Decompile"), side);
    m_btnBuild = makeSideBtn(QStringLiteral("Build Project"), side);
    m_btnPort = makeSideBtn(QStringLiteral("Port"), side);
    m_btnCopy = makeSideBtn(QStringLiteral("Copy to Game Folder"), side);
    m_btnRun = makeSideBtn(QStringLiteral("Run Executable"), side);
    for (auto* b : {m_btnLoad, m_btnLift, m_btnBuild, m_btnPort, m_btnCopy, m_btnRun})
        sideLay->addWidget(b);

    auto* setH = new QLabel(QStringLiteral("RECOMPILER SETTINGS"), side);
    setH->setObjectName(QStringLiteral("section"));
    sideLay->addSpacing(8);
    sideLay->addWidget(setH);
    /* Graphics API: Modify.exe */
    /* API + controller moved to per-game Modify.exe */

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
    auto* consolePage = new QWidget(right);
    auto* cLay = new QVBoxLayout(consolePage);
    cLay->setContentsMargins(0, 0, 0, 0);
    m_console = new QTextEdit(consolePage);
    m_console->setReadOnly(true);
    cLay->addWidget(m_console, 1);
    auto* cmdRow = new QHBoxLayout;
    auto* prompt = new QLabel(QStringLiteral("$"), consolePage);
    prompt->setStyleSheet(QStringLiteral("color:#00e5ff;font-weight:700;font-family:Consolas;"));
    m_consoleIn = new QLineEdit(consolePage);
    m_consoleIn->setPlaceholderText(QStringLiteral("help | status | clear | stop | -version"));
    m_consoleIn->setStyleSheet(QStringLiteral("background:#0d131f;border:1px solid #1a2a40;border-radius:6px;padding:6px;color:#e2e8f0;font-family:Consolas;"));
    cmdRow->addWidget(prompt);
    cmdRow->addWidget(m_consoleIn, 1);
    cLay->addLayout(cmdRow);
    m_tabs->addTab(consolePage, QStringLiteral("Console"));
    connect(m_consoleIn, &QLineEdit::returnPressed, this, &MainWindow::onConsoleSubmit);

    m_report = new QTextEdit(right);
    m_report->setReadOnly(true);
    m_tabs->addTab(m_report, QStringLiteral("Report"));

    m_roadmap = new QTextEdit(right);
    m_roadmap->setReadOnly(true);
    m_roadmap->setHtml(QStringLiteral(
        "<div style='font-family:Segoe UI;color:#c5d4e8;padding:10px'>"
        "<h2 style='color:#00e5ff'>Roadmap</h2>"
        "<ul>"
        "<li>Qt 6 host UI + AppData settings</li>"
        "<li>Unified runtime: ps3rt.dll (+ vulkan-1.dll)</li>"
        "<li>Vulkan 1.1 present + FIFO drain path</li>"
        "<li>HLE progress + import recovery</li>"
        "<li>Title-driven GCM streams</li>"
        "</ul></div>"));
    m_tabs->addTab(m_roadmap, QStringLiteral("Roadmap"));

    m_requirements = new QTextEdit(right);
    m_requirements->setReadOnly(true);
    loadRequirementsHtml();
    m_tabs->addTab(m_requirements, QStringLiteral("Requirements"));

    /* Controller mapping: keymap.json + Modify.exe */

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
    connect(m_btnPort, &QPushButton::clicked, this, &MainWindow::onPort);
    connect(m_btnBuild, &QPushButton::clicked, this, &MainWindow::onBuild);
    connect(m_btnCopy, &QPushButton::clicked, this, &MainWindow::onCopy);
    connect(m_btnRun, &QPushButton::clicked, this, &MainWindow::onRun);
    connect(m_threads, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onThreadsChanged);

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
    m_legal->setHtml(ppsx33_legal_html());
}

void MainWindow::loadRequirementsHtml() {
    m_requirements->setHtml(QStringLiteral(
        "<div style='font-family:Segoe UI;color:#c5d4e8;padding:12px;line-height:1.55'>"
        "<h2 style='color:#00e5ff'>Game runtime requirements</h2>"
        "<p>Applies to <b>running</b> a generated title EXE — not to building the recompiler UI.</p>"
        "<ul>"
        "<li><b>OS:</b> Windows x64</li>"
        "<li><b>Graphics:</b> Vulkan 1.1 capable GPU and driver</li>"
        "<li><b>OpenGL 4.5:</b> both Vulkan 1.1 and OpenGL 4.5 built; choose in Modify.exe</li>"
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
    /* graphics backend chosen in Modify.exe */
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
        "QMainWindow { background:#080b11; }"
        "QFrame#sidebar { background:#0a101c; border-right:1px solid #152033; }"
        "QFrame#logoBox { background:#0d1524; border:1px solid #1a2a44; border-radius:14px; padding:10px; }"
        "QLabel#sideTitle { color:#00e5ff; font-size:18px; font-weight:800; letter-spacing:1px; }"
        "QLabel#sideSub { color:#6b83a6; font-size:10px; letter-spacing:1px; }"
        "QLabel#section { color:#5b7396; font-size:10px; font-weight:700; letter-spacing:1px; margin-top:6px; }"
        "QLabel#fieldLabel { color:#7f95b5; font-size:10px; font-weight:600; }"
        "QLabel#engineReady { background:#0c1f18; color:#3ddc97; border:1px solid #1a3d30; border-radius:8px; padding:10px; font-weight:700; letter-spacing:1px; }"
        "QLabel#foot { color:#4d617d; font-size:10px; }"
        "QLabel#targetBar { background:#0a1220; color:#00e5ff; border:1px solid #1a2f4d; border-radius:8px; padding:10px; font-weight:700; letter-spacing:1px; }"
        "QLabel#statusLine { color:#7f95b5; font-size:11px; }"
        "QPushButton { background:#0f1828; color:#d5e3f7; border:1px solid #243652; border-radius:10px; text-align:left; padding-left:14px; }"
        "QPushButton:hover { border-color:#00e5ff; background:#152238; }"
        "QPushButton:disabled { color:#4a5d78; background:#0a1018; border-color:#152033; }"
        "QPushButton:pressed { background:#00e5ff; color:#061018; }"
        "QComboBox { background:#0f1828; color:#d5e3f7; border:1px solid #243652; border-radius:8px; padding:6px 10px; }"
        "QComboBox QAbstractItemView { background:#0f1828; color:#d5e3f7; selection-background-color:#1e3a5f; }"
        "QTabWidget::pane { border:1px solid #1a2a40; border-radius:10px; background:#080e18; }"
        "QTabBar::tab { background:#0a101c; color:#7f95b5; padding:10px 16px; margin-right:2px; border-top-left-radius:8px; border-top-right-radius:8px; }"
        "QTabBar::tab:selected { background:#121c2e; color:#e8eef7; }"
        "QTabBar::tab:hover { color:#00e5ff; }"
        "QTextEdit { background:#080e18; color:#c5d4e8; border:none; font-family:Consolas,'Cascadia Mono',monospace; font-size:12px; padding:8px; }"
        "QLineEdit { background:#0f1828; color:#e8eef7; border:1px solid #243652; border-radius:6px; padding:6px; }"
        "QProgressBar { background:#121a2b; border:none; }"
        "QProgressBar::chunk { background:#00e5ff; }"
        "QScrollArea { background:transparent; border:none; }"
    ));
}

void MainWindow::showEvent(QShowEvent* e) {
    QMainWindow::showEvent(e);
    static bool once = false;
    if (!once) { once = true; playIntro(); if (m_logoAnim) m_logoAnim->start(); }
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
    QList<QPushButton*> btns = { m_btnLoad, m_btnLift, m_btnBuild, m_btnPort, m_btnCopy, m_btnRun };
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



void MainWindow::setTargetLabel(const QString& t) { m_target->setText(t); }
void MainWindow::appendLog(const QString& s) { m_console->append(s); m_status->setText(s.left(120)); }
void MainWindow::setBusy(bool b) { m_busy = b; m_progress->setVisible(b); refreshButtons(); }

void MainWindow::refreshButtons() {
    m_btnLoad->setEnabled(!m_busy);
    m_btnLift->setEnabled(!m_busy && m_canLift);
    m_btnBuild->setEnabled(!m_busy && m_canBuild);
    if (m_btnPort) m_btnPort->setEnabled(!m_busy && m_canPort);
    m_btnCopy->setEnabled(!m_busy && m_canCopy);
    m_btnRun->setEnabled(!m_busy && m_canRun);
}

void MainWindow::onGfxChanged(int) {
    Settings::instance().setGraphicsBackend(QStringLiteral("Vulkan")); /* dual-API in game Modify.exe */
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
        QStringLiteral("ELF (*.elf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".elf"), Qt::CaseInsensitive)) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Only .elf files are supported."));
        return;
    }
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
    if (m_projectDir.isEmpty() || m_busy) return;
    setBusy(true);
    m_progress->setRange(0, 0);
    m_progress->setVisible(true);
    m_status->setText(QStringLiteral("Decompile live… (lift + analysis)"));
    m_jobKind = QStringLiteral("lift");
    m_liveStartMs = QDateTime::currentMSecsSinceEpoch();
    if (!m_liveTimer) {
        m_liveTimer = new QTimer(this);
        connect(m_liveTimer, &QTimer::timeout, this, &MainWindow::pollLiveBuild);
    }
    m_liveTimer->start(300);
    startJob(QStringLiteral("lift"));
}


void MainWindow::onBuild() {
    if (m_projectDir.isEmpty() || m_busy) return;
    setBusy(true);
    m_progress->setVisible(true);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_liveLogPath = m_projectDir + QStringLiteral("/output/build_live.log");
    QFile::remove(m_liveLogPath);
    m_buildTotal = 0;
    m_liveStartMs = QDateTime::currentMSecsSinceEpoch();
    m_jobKind = QStringLiteral("build");
    if (!m_liveTimer) {
        m_liveTimer = new QTimer(this);
        connect(m_liveTimer, &QTimer::timeout, this, &MainWindow::pollLiveBuild);
    }
    m_liveTimer->start(200);
    startJob(QStringLiteral("build"));
}


void MainWindow::onCopy() {
    QString start = Settings::instance().lastGameFolderPath();
    QString picked = QFileDialog::getExistingDirectory(this, QStringLiteral("Select game folder (contains USRDIR or EBOOT.BIN)"), start);
    if (picked.isEmpty()) return;
    Settings::instance().setLastGameFolderPath(picked);
    Settings::instance().save();

    /* Deploy next to EBOOT.BIN inside USRDIR (not the disc root alone). */
    auto resolveUsrdir = [](const QString& root) -> QString {
        const QStringList candidates = {
            root,
            root + QStringLiteral("/USRDIR"),
            root + QStringLiteral("/PS3_GAME/USRDIR"),
        };
        for (const QString& c : candidates) {
            if (QFile::exists(c + QStringLiteral("/EBOOT.BIN")) ||
                QFile::exists(c + QStringLiteral("/EBOOT.elf")) ||
                QFile::exists(c + QStringLiteral("/eboot.bin")))
                return QDir(c).absolutePath();
        }
        if (QDir(root + QStringLiteral("/USRDIR")).exists())
            return QDir(root + QStringLiteral("/USRDIR")).absolutePath();
        if (QDir(root + QStringLiteral("/PS3_GAME/USRDIR")).exists())
            return QDir(root + QStringLiteral("/PS3_GAME/USRDIR")).absolutePath();
        return QDir(root).absolutePath();
    };
    const QString dest = resolveUsrdir(picked);
    appendLog(QStringLiteral("[ui] deploy target USRDIR=%1").arg(dest));

    QString dist = QCoreApplication::applicationDirPath();
    for (const char* d : {"ps3rt.dll","vulkan-1.dll","ps3core.dll"}) {
        QString src = QDir(dist).filePath(QString::fromUtf8(d));
        if (QFile::exists(src)) {
            QString to = QDir(dest).filePath(QString::fromUtf8(d));
            QFile::remove(to);
            QFile::copy(src, to);
        }
    }
    QDir out(QDir(m_projectDir).filePath(QStringLiteral("output")));
    for (const auto& e : out.entryList(QStringList() << QStringLiteral("*.exe"), QDir::Files)) {
        QString to = QDir(dest).filePath(e);
        QFile::remove(to);
        QFile::copy(out.filePath(e), to);
        m_exePath = to;
    }
    /* Always ship memory image + input map + Modify next to EXE */
    for (const char* extra : {"guest_image.bin", "keymap.json", "Modify.exe", "ppsx33_tag.txt"}) {
        const QString src = out.filePath(QString::fromUtf8(extra));
        if (QFile::exists(src)) {
            const QString to = QDir(dest).filePath(QString::fromUtf8(extra));
            QFile::remove(to);
            if (QFile::copy(src, to))
                appendLog(QStringLiteral("[ui] deploy %1").arg(QString::fromUtf8(extra)));
        } else {
            appendLog(QStringLiteral("[ui] missing in output/: %1").arg(QString::fromUtf8(extra)));
        }
    }
    m_gameRoot = dest;
    m_canRun = !m_exePath.isEmpty();
    appendLog(QStringLiteral("[ui] copied EXE+DLLs+guest_image next to EBOOT in %1").arg(dest));
    refreshButtons();
}

void MainWindow::onRun() {
    if (m_exePath.isEmpty()) return;
    QProcess::startDetached(m_exePath, {}, m_gameRoot);
    appendLog(QStringLiteral("[ui] launched %1").arg(m_exePath));
}

void MainWindow::onConsoleSubmit() {
    if (!m_consoleIn) return;
    const QString cmd = m_consoleIn->text().trimmed();
    m_consoleIn->clear();
    if (cmd.isEmpty()) return;
    appendLog(QStringLiteral("$ %1").arg(cmd));
    const QString c = cmd.toLower();
    if (c == QStringLiteral("help")) {
        appendLog(QStringLiteral("Commands: help, status, clear, stop, -version"));
    } else if (c == QStringLiteral("clear")) {
        m_console->clear();
    } else if (c == QStringLiteral("status")) {
        appendLog(QStringLiteral("API=%1 threads=%2 project=%3")
            .arg(QStringLiteral("Vulkan+OpenGL (Modify.exe)"))
            .arg(m_threads ? m_threads->currentText() : QString())
            .arg(m_projectDir.isEmpty() ? QStringLiteral("(none)") : m_projectDir));
    } else if (c == QStringLiteral("stop")) {
        NativeBridge::instance().cancelBuild();
        setBusy(false);
        appendLog(QStringLiteral("Stop: cancel build / clear busy"));
    } else if (c == QStringLiteral("-version") || c == QStringLiteral("version")) {
        appendLog(QStringLiteral("PPSX33 %1").arg(QStringLiteral(PPSX33_VERSION_STRING)));
    } else {
        appendLog(QStringLiteral("Unknown command. Type help."));
    }
}

void MainWindow::pollLiveBuild() {
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_liveStartMs;
    if (m_jobKind == QStringLiteral("lift")) {
        m_progress->setRange(0, 0);
        m_status->setText(QStringLiteral("Decompile live… %1s").arg(elapsed / 1000));
        return;
    }
    QFile f(m_liveLogPath);
    if (!f.open(QIODevice::ReadOnly)) {
        m_status->setText(QStringLiteral("Build starting… %1s").arg(elapsed / 1000));
        return;
    }
    const QString text = QString::fromUtf8(f.readAll());
    // Live console tail
    const QStringList lines = text.split(QChar('\n'), Qt::SkipEmptyParts);
    if (!lines.isEmpty()) {
        static QString s_last;
        const QString last = lines.last().trimmed();
        if (last != s_last) {
            s_last = last;
            m_console->append(last);
        }
    }
    if (m_buildTotal <= 0) {
        QRegularExpression re(QStringLiteral("BUILD_TOTAL=([0-9]+)"));
        auto m = re.match(text);
        if (m.hasMatch()) m_buildTotal = m.captured(1).toInt();
    }
    const int done = text.count(QStringLiteral("COMPILE ")) + text.count(QStringLiteral("LINK "));
    if (m_buildTotal > 0) {
        int pct = qMin(99, (done * 100) / qMax(1, m_buildTotal));
        m_progress->setRange(0, 100);
        m_progress->setValue(pct);
        QString eta = QStringLiteral("…");
        if (pct > 2 && elapsed > 500) {
            const qint64 totalMs = elapsed * 100 / pct;
            const qint64 left = qMax(0LL, totalMs - elapsed);
            eta = QStringLiteral("%1s left").arg(left / 1000);
        }
        m_status->setText(QStringLiteral("Build %1% (%2/%3) ETA %4").arg(pct).arg(done).arg(m_buildTotal).arg(eta));
    } else {
        m_progress->setRange(0, 0);
        m_status->setText(QStringLiteral("Build live… %1s").arg(elapsed / 1000));
    }
}


void MainWindow::startJob(const QString& kind) {
    if (m_jobThread) {
        m_jobThread->quit();
        m_jobThread->wait(1000);
        m_jobThread->deleteLater();
        m_jobThread = nullptr;
        m_job = nullptr;
    }
    m_jobThread = new QThread(this);
    m_job = new JobWorker();
    m_job->moveToThread(m_jobThread);
    connect(m_jobThread, &QThread::finished, m_job, &QObject::deleteLater);
    connect(m_job, &JobWorker::finished, this, &MainWindow::onJobFinished);
    const QString pd = m_projectDir;
    const QString dist = QCoreApplication::applicationDirPath();
    if (kind == QStringLiteral("lift")) {
        connect(m_jobThread, &QThread::started, m_job, [this, pd]() { m_job->lift(pd); });
    } else {
        connect(m_jobThread, &QThread::started, m_job, [this, pd, dist]() { m_job->build(pd, dist); });
    }
    m_jobThread->start();
}

void MainWindow::onJobFinished(bool ok, QString log, QString kind) {
    if (m_liveTimer) m_liveTimer->stop();
    if (!log.isEmpty()) {
        m_report->setPlainText(log);
        appendLog(log.left(500));
    }
    if (kind == QStringLiteral("lift")) {
        m_canBuild = ok;
        appendLog(ok ? QStringLiteral("[ui] decompile ok") : QStringLiteral("[ui] decompile failed"));
        if (!ok) QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Decompile failed. See Report."));
    } else {
        m_canPort = ok;
        m_canCopy = false; // after Port only
        if (ok) {
            m_progress->setValue(100);
            appendLog(QStringLiteral("[ui] build ok — click Port to emit Modify.exe"));
        } else {
            appendLog(QStringLiteral("[ui] build failed"));
            QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Build failed. See Report / Console."));
        }
    }
    setBusy(false);
    refreshButtons();
    if (m_jobThread) {
        m_jobThread->quit();
        m_jobThread->wait(500);
    }
}


void MainWindow::onPort() {
    appendLog(QStringLiteral("[port] extracting Modify.exe + binding unlock key…"));
    if (!NativeBridge::instance().portModify(m_projectDir)) {
        appendLog(QStringLiteral("[port] failed: %1").arg(NativeBridge::instance().lastError()));
        return;
    }
    appendLog(QStringLiteral("[port] Modify.exe ready next to game output. Copy to Game Folder unlocked."));
    m_canCopy = true;
    m_canPort = false;
    refreshButtons();
}
