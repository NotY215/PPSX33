#include "MainWindow.h"
#include "RenderWorkbench.h"
#include "TermsDialog.h"
#include "Settings.h"
#include "AppVersion.h"
#include <QApplication>
#include <QMessageBox>
#include <cstdio>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("PPSX33"));
    app.setOrganizationName(QStringLiteral("PPSX33"));
    app.setApplicationVersion(QStringLiteral(PPSX33_VERSION_STRING));

    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("-version")) || args.contains(QStringLiteral("--version"))) {
        std::printf("PPSX33 %s\n", PPSX33_VERSION_STRING);
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
