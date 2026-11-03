#include "MainWindow.h"
#include "TermsDialog.h"
#include "Settings.h"
#include "AppVersion.h"
#include <QMessageBox>
#if defined(_WIN32)
#include <windows.h>
#endif

static void checkHostGraphics(QWidget* parent) {
    QStringList miss;
#if defined(_WIN32)
    HMODULE vk = LoadLibraryA("vulkan-1.dll");
    if (!vk) miss << "Vulkan 1.2 loader (vulkan-1.dll)";
    else FreeLibrary(vk);
    HMODULE gl = LoadLibraryA("opengl32.dll");
    if (!gl) miss << "OpenGL (opengl32.dll)";
    else {
        /* 4.5 presence cannot be known without a context; warn only if DLL missing */
        FreeLibrary(gl);
    }
#endif
    if (!miss.isEmpty()) {
        QMessageBox::warning(parent, QStringLiteral("Graphics capability"),
            QStringLiteral("This PC may be missing:\n- %1\n\n"
                           "PPSX33 needs Vulkan 1.2 and/or OpenGL 4.5 for game present.\n"
                           "You can still use the recompiler UI.")
                .arg(miss.join("\n- ")));
    }
}

#include <QApplication>
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

    MainWindow w;
    checkHostGraphics(&w);
    w.show();
    return app.exec();
}
