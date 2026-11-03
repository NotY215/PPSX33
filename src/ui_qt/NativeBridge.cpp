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


#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QTextStream>

static QString makeUnlockKey() {
    // High-entropy printable token (C-string safe for unlock.inc); unique per call/ms
    static const char alphabet[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz0123456789!@#$%^&*-_=+";
    const int n = (int)sizeof(alphabet) - 1;
    QString out = QStringLiteral("P33-");
    auto* rng = QRandomGenerator::global();
    quint64 mix = (quint64)QDateTime::currentMSecsSinceEpoch() ^ (quint64)rng->generate64();
    for (int i = 0; i < 24; ++i) {
        mix = mix * 6364136223846793005ull + 1ull;
        out.append(QChar(alphabet[(mix >> 33) % (quint64)n]));
    }
    return out;
}

bool NativeBridge::portModify(const QString& projectDir) {
    m_err.clear();
    if (projectDir.isEmpty()) {
        m_err = QStringLiteral("no project");
        return false;
    }
    QDir out(QDir(projectDir).filePath(QStringLiteral("output")));
    if (!out.exists()) {
        m_err = QStringLiteral("output folder missing — build first");
        return false;
    }

    // Resolve game EXE name
    QString gameExe;
    for (const QFileInfo& fi : out.entryInfoList(QStringList() << QStringLiteral("*.exe"), QDir::Files)) {
        const QString n = fi.fileName();
        if (n.compare(QStringLiteral("Modify.exe"), Qt::CaseInsensitive) == 0) continue;
        gameExe = n;
        break;
    }
    if (gameExe.isEmpty()) {
        m_err = QStringLiteral("game EXE not found in output");
        return false;
    }

    // Key: prefer unlock.inc from build, else generate + write
    QString key;
    QFile uk(out.filePath(QStringLiteral("unlock.inc")));
    if (uk.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString line = QString::fromUtf8(uk.readAll());
        const int a = line.indexOf('"');
        const int b = line.lastIndexOf('"');
        if (a >= 0 && b > a) key = line.mid(a + 1, b - a - 1);
    }
    if (key.isEmpty()) {
        key = makeUnlockKey();
        if (uk.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream ts(&uk);
            ts << "#define PPSX33_UNLOCK_KEY \"" << key << "\"\n";
        }
    }

    // Temp encrypted lockfile (obfuscated, not full AES library — high entropy token store)
    const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString lockPath = QDir(tempDir).filePath(QStringLiteral("ppsx33_%1.lockfile").arg(QDateTime::currentMSecsSinceEpoch()));
    {
        QFile lf(lockPath);
        if (lf.open(QIODevice::WriteOnly)) {
            QByteArray raw = key.toUtf8();
            QByteArray enc;
            enc.reserve(raw.size());
            const quint8 salt = (quint8)(QDateTime::currentMSecsSinceEpoch() & 0xFF);
            enc.append((char)salt);
            for (unsigned char c : raw) enc.append((char)(c ^ salt ^ 0xA5));
            lf.write(enc.toBase64());
        }
    }

    // Copy Modify.exe from next to PPSX33.exe (built host tool embedded in dist)
    const QString srcMod = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("Modify.exe"));
    const QString dstMod = out.filePath(QStringLiteral("Modify.exe"));
    if (!QFile::exists(srcMod)) {
        m_err = QStringLiteral("Modify.exe missing next to PPSX33.exe — rebuild host target PPSX33Modify");
        QFile::remove(lockPath);
        return false;
    }
    QFile::remove(dstMod);
    if (!QFile::copy(srcMod, dstMod)) {
        m_err = QStringLiteral("failed to copy Modify.exe");
        QFile::remove(lockPath);
        return false;
    }

    // Launch sidecar bound to this game + key
    QFile lj(out.filePath(QStringLiteral("ppsx33_launch.json")));
    if (lj.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream ts(&lj);
        ts << "{\n  \"game\": \"" << gameExe << "\",\n  \"key\": \"" << key
           << "\",\n  \"api\": \"Vulkan\",\n  \"apis\": [\"Vulkan\",\"OpenGL\"]\n}\n";
    }

    // Default keymap if missing
    const QString km = out.filePath(QStringLiteral("keymap.json"));
    if (!QFile::exists(km)) {
        QFile f(km);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write("{\n  \"Cross\": \"Z\", \"Circle\": \"X\", \"Square\": \"A\", \"Triangle\": \"S\",\n"
                    "  \"Start\": \"V\", \"Select\": \"Space\"\n}\n");
        }
    }

    QFile::remove(lockPath); // port finished
    return true;
}
