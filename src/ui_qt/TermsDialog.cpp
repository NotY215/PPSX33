#include "TermsDialog.h"
#include "Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>

static QString termsHtml() {
    return QStringLiteral(
        "<div style='font-family:Segoe UI,sans-serif;color:#c5d4e8;line-height:1.55;padding:8px'>"
        "<h2 style='color:#00e5ff;margin-top:0'>PPSX33 — Terms and Conditions</h2>"
        "<p style='color:#8ba3c7'><b>Effective date:</b> 2026-10-28</p>"
        "<p>By using PPSX33 (GUI, core library, runtime, and generated host binaries branded "
        "<b>PPSX33</b>), you agree to these Terms. If you do not agree, do not use the Software.</p>"
        "<h3 style='color:#7dd3fc'>1. Nature of the Software</h3>"
        "<p>PPSX33 is an <b>independent research and engineering toolkit</b> (recompiler / HLE runtime) "
        "for technical study and personal experimentation.</p>"
        "<p>It is <b>not</b> affiliated with, endorsed by, or sponsored by Sony Interactive Entertainment "
        "or any PlayStation brand owner.</p>"
        "<p>Generated host executables and branding are tooling identifiers, not retail PlayStation products.</p>"
        "<h3 style='color:#7dd3fc'>2. License to PPSX33 code</h3>"
        "<p>Source and original assets are subject to the project LICENSE. You may study, modify, and build "
        "for lawful personal or research use.</p>"
        "<h3 style='color:#7dd3fc'>3. User responsibilities</h3>"
        "<ul>"
        "<li>You are solely responsible for obtaining game data and ensuring lawful use.</li>"
        "<li>PPSX33 does not decrypt protected content, distribute keys, or provide copyrighted game binaries.</li>"
        "<li>Do not use the Software to infringe intellectual property or other law.</li>"
        "</ul>"
        "<h3 style='color:#7dd3fc'>4. No warranty</h3>"
        "<p>THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND.</p>"
        "</div>");
}

TermsDialog::TermsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("PPSX33 — Terms"));
    setModal(true);
    resize(640, 520);
    setStyleSheet(QStringLiteral(
        "QDialog { background:#080b11; color:#e2e8f0; }"
        "QPushButton { background:#161e2e; border:1px solid #2a3d5c; border-radius:8px; padding:10px 18px; color:#e2e8f0; }"
        "QPushButton#accept { background:#0c1f18; border-color:#10b981; color:#34d399; font-weight:600; }"
        "QPushButton#decline { color:#f87171; border-color:#7f1d1d; }"
        "QTextBrowser { background:#0d131f; border:1px solid #1a2a40; border-radius:10px; padding:8px; }"));
    auto* lay = new QVBoxLayout(this);
    auto* head = new QLabel(QStringLiteral("Please review and accept the Terms to continue."), this);
    head->setStyleSheet(QStringLiteral("color:#94a3b8;"));
    lay->addWidget(head);
    auto* browser = new QTextBrowser(this);
    browser->setHtml(termsHtml());
    lay->addWidget(browser, 1);
    auto* row = new QHBoxLayout;
    auto* decline = new QPushButton(QStringLiteral("Decline"), this);
    decline->setObjectName(QStringLiteral("decline"));
    auto* accept = new QPushButton(QStringLiteral("Accept Terms"), this);
    accept->setObjectName(QStringLiteral("accept"));
    row->addStretch();
    row->addWidget(decline);
    row->addWidget(accept);
    lay->addLayout(row);
    connect(decline, &QPushButton::clicked, this, [this] { m_ok = false; reject(); });
    connect(accept, &QPushButton::clicked, this, [this] {
        m_ok = true;
        Settings::instance().setTermsAccepted(true, qEnvironmentVariable("USERNAME", QStringLiteral("user")),
                                              QStringLiteral("2026-10-28"));
        Settings::instance().save();
        QDialog::accept();
    });
}
