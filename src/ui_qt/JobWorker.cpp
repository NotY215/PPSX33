#include "JobWorker.h"
#include "NativeBridge.h"

void JobWorker::lift(const QString& projectDir) {
    QString log;
    bool ok = NativeBridge::instance().liftProject(projectDir, &log);
    emit finished(ok, log, QStringLiteral("lift"));
}

void JobWorker::build(const QString& projectDir, const QString& distDir) {
    NativeBridge::instance().beginBuild();
    QString log;
    bool ok = NativeBridge::instance().buildProject(projectDir, distDir, distDir, &log);
    emit finished(ok, log, QStringLiteral("build"));
}
