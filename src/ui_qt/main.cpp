#include "MainWindow.h"
#include "RenderWorkbench.h"
#include <QApplication>
#include <QFile>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("PPSX33"));
    app.setOrganizationName(QStringLiteral("PPSX33"));
    QStringList args = app.arguments();
    if (args.contains(QStringLiteral("-render"))) {
        RenderWorkbench w;
        w.show();
        return app.exec();
    }
    MainWindow w;
    w.show();
    return app.exec();
}
