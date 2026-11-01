#include "RenderWorkbench.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QTimer>

RenderWorkbench::RenderWorkbench(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("PPSX33 — Private mode"));
    resize(720, 520);
    setStyleSheet(QStringLiteral(
        "QDialog { background:#0b1220; color:#e2e8f0; }"
        "QPushButton { background:#1e293b; border:1px solid #334155; border-radius:8px; padding:8px 12px; }"
        "QPushButton:hover { border-color:#38bdf8; }"
        "QTextEdit { background:#0f172a; border:1px solid #1e293b; font-family:Consolas,monospace; }"
        "QCheckBox { spacing:8px; }"));
    auto* root = new QVBoxLayout(this);
    auto* welcome = new QLabel(QStringLiteral("Welcome to Private mode"), this);
    welcome->setStyleSheet(QStringLiteral("font-size:18px;color:#38bdf8;font-weight:600;"));
    root->addWidget(welcome);
    auto* row = new QHBoxLayout;
    for (const char* n : {"api", "rsx", "render", "resolution", "ps3rt"}) {
        auto* cb = new QCheckBox(QString::fromUtf8(n), this);
        cb->setChecked(true);
        row->addWidget(cb);
    }
    root->addLayout(row);
    auto* actions = new QHBoxLayout;
    auto* load = new QPushButton(QStringLiteral("Load ELF"), this);
    auto* decomp = new QPushButton(QStringLiteral("Decompile"), this);
    auto* build = new QPushButton(QStringLiteral("Build modules"), this);
    actions->addWidget(load);
    actions->addWidget(decomp);
    actions->addWidget(build);
    root->addLayout(actions);
    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    root->addWidget(m_console, 1);
    connect(build, &QPushButton::clicked, this, [this] {
        m_console->append(QStringLiteral("[render] module rebuild requested (hook CMake targets)"));
    });
    QTimer::singleShot(50, this, &RenderWorkbench::playWelcome);
}

void RenderWorkbench::playWelcome() {
    auto* fx = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(fx);
    auto* a = new QPropertyAnimation(fx, "opacity", this);
    a->setDuration(600);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}
