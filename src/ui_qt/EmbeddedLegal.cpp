#include "EmbeddedLegal.h"
#include <QProcessEnvironment>

QString ppsx33_legal_html() {
    QString user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USERNAME"));
    if (user.isEmpty())
        user = QProcessEnvironment::systemEnvironment().value(QStringLiteral("USER"), QStringLiteral("%USERNAME%"));
    return QStringLiteral(
        "<div style='font-family:Segoe UI,sans-serif;color:#c5d4e8;line-height:1.6;padding:16px'>"
        "<h2 style='color:#00e5ff;margin:0 0 10px 0'>PPSX33 — Legal Notice &amp; Project Position</h2>"
        "<p style='color:#94a3b8'><b>Last updated:</b> 2026-10-28</p>"
        "<p>This document describes what PPSX33 is, what it is not, and the conditions under which "
        "<b>%1</b> may use this repository and software. <b>Read it before using Load ELF, Build, or Run.</b></p>"
        "<h3 style='color:#7dd3fc'>Summary</h3>"
        "<ul>"
        "<li>Independent research recompiler / HLE toolkit — <b>not</b> affiliated with Sony Interactive Entertainment or any PlayStation brand.</li>"
        "<li>No bundled decryption keys; optional external decrypt tools are <b>user-supplied</b>.</li>"
        "<li>Current builds load <b>decrypted ELF</b> only on the load path.</li>"
        "<li>Use only <b>lawfully obtained</b> materials; do <b>not</b> redistribute game data.</li>"
        "<li>Host branding is <b>PPSX33</b> (tooling, not a retail port).</li>"
        "</ul>"
        "<h3 style='color:#7dd3fc'>What PPSX33 is</h3>"
        "<ol>"
        "<li>An engineering toolkit for studying PS3 executables via static lift + HLE runtime.</li>"
        "<li>A host-side presentation path (Vulkan 1.2 default, OpenGL secondary).</li>"
        "<li>Open-source research software under the project LICENSE.</li>"
        "</ol>"
        "<h3 style='color:#7dd3fc'>What PPSX33 is not</h3>"
        "<ol>"
        "<li>Not an official PlayStation emulator product.</li>"
        "<li>Not a service that distributes games, keys, or firmware.</li>"
        "<li>Not a warranty of compatibility with any specific title.</li>"
        "</ol>"
        "<h3 style='color:#7dd3fc'>Responsibilities for %1</h3>"
        "<ul>"
        "<li>%1 is responsible for the legality of any game data processed in their jurisdiction.</li>"
        "<li>Do not use PPSX33 to infringe intellectual property or violate export rules.</li>"
        "<li>Reports and logs may contain local paths — review before publishing.</li>"
        "</ul>"
        "<h3 style='color:#7dd3fc'>No warranty</h3>"
        "<p>THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, "
        "INCLUDING MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.</p>"
        "<h3 style='color:#7dd3fc'>Limitation of liability</h3>"
        "<p>IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM OR DAMAGES ARISING FROM USE OF THE SOFTWARE.</p>"
        "<p style='color:#8ba3c7'>Canonical sources: docs/legal/LEGAL.md, docs/legal/TERMS.md, and the repository LICENSE.</p>"
        "</div>").arg(user.toHtmlEscaped());
}
