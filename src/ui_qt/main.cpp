#include "MainWindow.h"
#include "RenderWorkbench.h"
#include "TermsDialog.h"
#include "Settings.h"
#include "AppVersion.h"
#include <QApplication>
#include <QMessageBox>
#include <cstdio>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("PPSX33"));
    app.setOrganizationName(QStringLiteral("PPSX33"));
    app.setApplicationVersion(QStringLiteral(PPSX33_VERSION_STRING));

    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("-version")) || args.contains(QStringLiteral("--version"))) {
        #ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
            FILE* f = nullptr;
            freopen_s(&f, "CONOUT$", "w", stdout);
        }
#endif
        std::printf("PPSX33 %s\n", PPSX33_VERSION_STRING);
        std::fflush(stdout);
        return 0;
    }

    Settings::instance().load();
    if (!Settings::instance().termsAccepted()) {
        TermsDialog td;
        if (td.exec() != QDialog::Accepted || !td.acceptedTerms())
            return 1;
    }

    if (args.contains(QStringLiteral("-render"))) {
        RenderWorkbench w;
        w.show();
        return app.exec();
    }

    MainWindow w;
    w.show();
    return app.exec();
}
