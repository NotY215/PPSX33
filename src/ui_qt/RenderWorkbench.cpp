#include "RenderWorkbench.h"
#include "NativeBridge.h"
#include "AppVersion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QCoreApplication>

RenderWorkbench::RenderWorkbench(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("PPSX33 · Private mode — %1").arg(QStringLiteral(PPSX33_VERSION_STRING)));
    resize(820, 600);
    setStyleSheet(QStringLiteral(
        "QDialog { background:#080b11; color:#e2e8f0; }"
        "QLabel#welcome { color:#00e5ff; font-size:20px; font-weight:700; }"
        "QLabel#target { background:#0d131f; border:1px solid #1a2a40; border-radius:8px; padding:10px; color:#00e5ff; }"
        "QPushButton { background:#161e2e; border:1px solid #2a3d5c; border-radius:8px; padding:9px 14px; color:#e2e8f0; }"
        "QPushButton:hover { border-color:#00e5ff; }"
        "QPushButton:disabled { color:#4b5563; }"
        "QTextEdit { background:#0d131f; border:1px solid #1a2a40; border-radius:8px; font-family:Consolas,monospace; color:#c5d4e8; }"
        "QCheckBox { spacing:8px; color:#cbd5e1; }"));

    auto* root = new QVBoxLayout(this);
    auto* welcome = new QLabel(QStringLiteral("Welcome to Private mode"), this);
    welcome->setObjectName(QStringLiteral("welcome"));
    root->addWidget(welcome);
    auto* sub = new QLabel(QStringLiteral("Regenerate host modules from analyzed ELF (rsx / render / resolution / api / ps3rt)."), this);
    sub->setWordWrap(true);
    sub->setStyleSheet(QStringLiteral("color:#94a3b8; font-size:12px;"));
    root->addWidget(sub);

    m_target = new QLabel(QStringLiteral("TARGET / NO ELF"), this);
    m_target->setObjectName(QStringLiteral("target"));
    root->addWidget(m_target);

    auto* mods = new QHBoxLayout;
    m_cbRsx = new QCheckBox(QStringLiteral("rsx"), this);
    m_cbRender = new QCheckBox(QStringLiteral("render"), this);
    m_cbRt = new QCheckBox(QStringLiteral("ps3rt"), this);
    m_cbRsx->setChecked(true);
    m_cbRender->setChecked(true);
    m_cbRt->setChecked(true);
    for (auto* c : {m_cbRsx, m_cbRender, m_cbRt}) mods->addWidget(c);
    root->addLayout(mods);

    auto* row = new QHBoxLayout;
    auto* load = new QPushButton(QStringLiteral("Load ELF"), this);
    auto* decomp = new QPushButton(QStringLiteral("Decompile"), this);
    auto* analyze = new QPushButton(QStringLiteral("Analyze"), this);
    auto* gen = new QPushButton(QStringLiteral("Generate DLLs"), this);
    row->addWidget(load);
    row->addWidget(decomp);
    row->addWidget(analyze);
    row->addWidget(gen);
    root->addLayout(row);

    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    root->addWidget(m_console, 1);
    log(QStringLiteral("Private mode · %1").arg(QStringLiteral(PPSX33_VERSION_STRING)));
    log(QStringLiteral("All module rebuilds require Load ELF → Decompile → Analyze → Generate for title-linked paths."));

    connect(load, &QPushButton::clicked, this, &RenderWorkbench::onLoadElf);
    connect(decomp, &QPushButton::clicked, this, &RenderWorkbench::onDecompile);
    connect(analyze, &QPushButton::clicked, this, &RenderWorkbench::onAnalyze);
    connect(gen, &QPushButton::clicked, this, &RenderWorkbench::onGenerate);

    if (!NativeBridge::instance().load())
        log(QStringLiteral("ps3core: %1").arg(NativeBridge::instance().lastError()));

    auto* fx = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fx);
    auto* a = new QPropertyAnimation(fx, "opacity", this);
    a->setDuration(650);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    QTimer::singleShot(30, a, [a] { a->start(QAbstractAnimation::DeleteWhenStopped); });
}

void RenderWorkbench::log(const QString& s) { m_console->append(s); }

void RenderWorkbench::onLoadElf() {
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Load ELF"),
        QString(), QStringLiteral("ELF (*.elf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(QStringLiteral(".elf"), Qt::CaseInsensitive)) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Only .elf files are supported."));
        return;
    }
    m_elf = path;
    m_lifted = m_analyzed = false;
    m_target->setText(QStringLiteral("TARGET / %1").arg(QFileInfo(path).fileName()));
    log(QStringLiteral("Loaded %1").arg(path));
}

void RenderWorkbench::onDecompile() {
    if (m_elf.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Load an .elf first."));
        return;
    }
    QString root = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("projects"));
    QDir().mkpath(root);
    QString game = QFileInfo(m_elf).completeBaseName();
    QString pd, err;
    if (!NativeBridge::instance().createProject(root, game, m_elf, &pd, &err)) {
        QMessageBox::critical(this, QStringLiteral("PPSX33"), err);
        return;
    }
    m_project = pd;
    QString logt;
    if (!NativeBridge::instance().liftProject(pd, &logt)) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Decompile failed."));
        log(logt);
        return;
    }
    m_lifted = true;
    log(QStringLiteral("Decompile ok → %1").arg(pd));
}

void RenderWorkbench::onAnalyze() {
    if (!m_lifted) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"), QStringLiteral("Decompile first."));
        return;
    }
    QDir code(QDir(m_project).filePath(QStringLiteral("codebase")));
    int kept = 0, removed = 0;
    for (const auto& fi : code.entryInfoList(QDir::Files)) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        const QByteArray body = f.read(65536);
        const bool useful = body.contains("gcm") || body.contains("rsx") || body.contains("FIFO")
            || body.contains("cellGcm") || body.contains("sys_rsx") || body.contains("HLE");
        if (!useful && fi.suffix() == QStringLiteral("cpp")) {
            f.close();
            if (QFile::remove(fi.absoluteFilePath())) ++removed;
        } else ++kept;
    }
    m_analyzed = true;
    log(QStringLiteral("Analyze: kept ~%1 sources, removed %2 non-HLE/RSX helpers").arg(kept).arg(removed));
}

void RenderWorkbench::onGenerate() {
    if (!m_analyzed) {
        QMessageBox::warning(this, QStringLiteral("PPSX33"),
            QStringLiteral("ELF-linked modules require Load ELF → Decompile → Analyze first."));
        return;
    }
    QStringList mods;
    if (m_cbRsx->isChecked()) mods << QStringLiteral("rsx");
    if (m_cbRender->isChecked()) mods << QStringLiteral("render");
    if (m_cbRt->isChecked()) mods << QStringLiteral("ps3rt");
    log(QStringLiteral("Generate requested: %1 (hook host CMake targets / copy from dist/)").arg(mods.join(QStringLiteral(", "))));
    QString dist = QCoreApplication::applicationDirPath();
    for (const auto& m : mods) {
        QString dll = dist + QStringLiteral("/") + m + QStringLiteral(".dll");
        if (QFile::exists(dll))
            log(QStringLiteral("  present: %1").arg(dll));
        else
            log(QStringLiteral("  missing (rebuild host): %1").arg(dll));
    }
}
