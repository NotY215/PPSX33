#include "EmbeddedTerms.h"
#include <QProcessEnvironment>

QString ppsx33_terms_html() {
    QString user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USERNAME"));
    if (user.isEmpty())
        user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USER"), QStringLiteral("%USERNAME%"));
    return QStringLiteral(
        "<div style='font-family:Segoe UI,sans-serif;color:#c5d4e8;line-height:1.55;padding:12px'>"
        "<h2 style='color:#00e5ff;margin-top:0'>PPSX33 — Terms and Conditions</h2>"
        "<p style='color:#94a3b8'><b>Effective date:</b> 2026-10-28</p>"
        "<p>By cloning, building, installing, or using PPSX33, <b>%1</b> agrees to these Terms. "
        "If %1 does not agree, do not use the Software.</p>"
        "<h3 style='color:#7dd3fc'>1. Nature of the Software</h3>"
        "<p>PPSX33 is an independent research and engineering toolkit (recompiler / HLE runtime) "
        "for technical study and personal experimentation.</p>"
        "<p>It is not affiliated with, endorsed by, or sponsored by Sony Interactive Entertainment "
        "or any PlayStation brand owner.</p>"
        "<p>Generated host executables and branding are tooling identifiers, not retail products.</p>"
        "<h3 style='color:#7dd3fc'>2. License</h3>"
        "<p>Source and original assets are subject to the project LICENSE. %1 may study, modify, and "
        "build for lawful personal or research use.</p>"
        "<h3 style='color:#7dd3fc'>3. Responsibilities of %1</h3>"
        "<ul>"
        "<li>%1 is solely responsible for obtaining game data and ensuring lawful use.</li>"
        "<li>PPSX33 does not decrypt protected content, distribute keys, or provide copyrighted game binaries.</li>"
        "<li>%1 must not use the Software to infringe intellectual property or other law.</li>"
        "</ul>"
        "<h3 style='color:#7dd3fc'>4. No warranty</h3>"
        "<p>THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND.</p>"
        "</div>").arg(user.toHtmlEscaped());
}
