#include "NativeBridge.h"
#include <QCoreApplication>
#include <QDir>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

NativeBridge& NativeBridge::instance() {
    static NativeBridge n;
    return n;
}

bool NativeBridge::load(const QString& dir) {
    if (m_ok) return true;
    QString base = dir.isEmpty() ? QCoreApplication::applicationDirPath() : dir;
    QString path = QDir(base).filePath(
#ifdef _WIN32
        "ps3core.dll"
#else
        "libps3core.so"
#endif
    );
#ifdef _WIN32
    HMODULE h = LoadLibraryW(reinterpret_cast<LPCWSTR>(path.utf16()));
    if (!h) {
        m_err = QStringLiteral("LoadLibrary failed: %1").arg(path);
        return false;
    }
    m_lib = h;
    auto sym = [&](const char* n) { return (void*)GetProcAddress(h, n); };
#else
    void* h = dlopen(path.toUtf8().constData(), RTLD_NOW);
    if (!h) {
        m_err = QString::fromUtf8(dlerror());
        return false;
    }
    m_lib = h;
    auto sym = [&](const char* n) { return dlsym(h, n); };
#endif
    m_version = (decltype(m_version))sym("ps3_core_version");
    m_create  = (decltype(m_create))sym("ps3_create_project");
    m_lift    = (decltype(m_lift))sym("ps3_lift_project");
    m_begin   = (decltype(m_begin))sym("ps3_begin_build");
    m_cancel  = (decltype(m_cancel))sym("ps3_cancel_build");
    m_build   = (decltype(m_build))sym("ps3_build_project");
    if (!m_version || !m_create || !m_lift || !m_build) {
        m_err = QStringLiteral("ps3core exports missing");
        return false;
    }
    m_ok = true;
    return true;
}

int NativeBridge::version() const { return m_version ? m_version() : 0; }

bool NativeBridge::createProject(const QString& root, const QString& game, const QString& elf,
                                 QString* projectDir, QString* err) {
    if (!m_create) return false;
    char pd[1024]{}, er[1024]{};
    int rc = m_create(root.toUtf8().constData(), game.toUtf8().constData(), elf.toUtf8().constData(),
                      pd, (int)sizeof(pd), er, (int)sizeof(er));
    if (projectDir) *projectDir = QString::fromUtf8(pd);
    if (err) *err = QString::fromUtf8(er);
    return rc == 0;
}

bool NativeBridge::liftProject(const QString& projectDir, QString* log) {
    if (!m_lift) return false;
    char buf[65536]{};
    int rc = m_lift(projectDir.toUtf8().constData(), buf, (int)sizeof(buf));
    if (log) *log = QString::fromUtf8(buf);
    return rc == 0;
}

void NativeBridge::beginBuild() { if (m_begin) m_begin(); }
void NativeBridge::cancelBuild() { if (m_cancel) m_cancel(); }

bool NativeBridge::buildProject(const QString& projectDir, const QString& runtimeDir,
                                const QString& compilersDir, QString* log) {
    if (!m_build) return false;
    char buf[65536]{};
    int rc = m_build(projectDir.toUtf8().constData(), runtimeDir.toUtf8().constData(),
                     compilersDir.toUtf8().constData(), buf, (int)sizeof(buf));
    if (log) *log = QString::fromUtf8(buf);
    return rc == 0;
}
