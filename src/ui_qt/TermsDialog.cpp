#include "TermsDialog.h"
#include "EmbeddedTerms.h"
#include "Settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QProcessEnvironment>

TermsDialog::TermsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("PPSX33 — Terms and Conditions"));
    setModal(true);
    resize(700, 560);
    setStyleSheet(QStringLiteral(
        "QDialog { background:#080b11; color:#e2e8f0; }"
        "QPushButton { background:#161e2e; border:1px solid #2a3d5c; border-radius:8px; padding:10px 18px; color:#e2e8f0; }"
        "QPushButton#accept { background:#0c1f18; border-color:#10b981; color:#34d399; font-weight:600; }"
        "QPushButton#accept:disabled { color:#4b5563; border-color:#1f2937; background:#0a1018; }"
        "QPushButton#decline { color:#f87171; border-color:#7f1d1d; }"
        "QTextBrowser { background:#0d131f; border:1px solid #1a2a40; border-radius:10px; padding:8px; }"
        "QCheckBox { color:#cbd5e1; spacing:8px; }"));
    auto* lay = new QVBoxLayout(this);
    QString user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USERNAME"), QStringLiteral("%USERNAME%"));
    auto* head = new QLabel(QStringLiteral("Signed in as <b>%1</b> — read and accept the Terms to continue.").arg(user.toHtmlEscaped()), this);
    head->setStyleSheet(QStringLiteral("color:#94a3b8;"));
    head->setTextFormat(Qt::RichText);
    lay->addWidget(head);
    auto* browser = new QTextBrowser(this);
    browser->setHtml(ppsx33_terms_html());
    lay->addWidget(browser, 1);
    auto* box = new QCheckBox(QStringLiteral("I, %1, have read and agree to these Terms and Conditions.").arg(user), this);
    lay->addWidget(box);
    auto* row = new QHBoxLayout;
    auto* decline = new QPushButton(QStringLiteral("Decline"), this);
    decline->setObjectName(QStringLiteral("decline"));
    auto* accept = new QPushButton(QStringLiteral("Accept Terms"), this);
    accept->setObjectName(QStringLiteral("accept"));
    accept->setEnabled(false);
    row->addStretch();
    row->addWidget(decline);
    row->addWidget(accept);
    lay->addLayout(row);
    connect(box, &QCheckBox::toggled, accept, &QPushButton::setEnabled);
    connect(decline, &QPushButton::clicked, this, [this] { m_ok = false; reject(); });
    connect(accept, &QPushButton::clicked, this, [this, user] {
        m_ok = true;
        Settings::instance().setTermsAccepted(true, user, QStringLiteral("2026-10-28"));
        Settings::instance().save();
        QDialog::accept();
    });
}
