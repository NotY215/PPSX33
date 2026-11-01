#include "Settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

Settings& Settings::instance() {
    static Settings s;
    return s;
}

QMap<QString, QString> Settings::defaultKeyMap() {
    return {
        {QStringLiteral("LeftStickLeft"), QStringLiteral("Left")},
        {QStringLiteral("LeftStickDown"), QStringLiteral("Down")},
        {QStringLiteral("LeftStickRight"), QStringLiteral("Right")},
        {QStringLiteral("LeftStickUp"), QStringLiteral("Up")},
        {QStringLiteral("RightStickLeft"), QStringLiteral("H")},
        {QStringLiteral("RightStickDown"), QStringLiteral("J")},
        {QStringLiteral("RightStickRight"), QStringLiteral("K")},
        {QStringLiteral("RightStickUp"), QStringLiteral("U")},
        {QStringLiteral("Start"), QStringLiteral("V")},
        {QStringLiteral("Select"), QStringLiteral("Space")},
        {QStringLiteral("PsButton"), QStringLiteral("Backspace")},
        {QStringLiteral("Square"), QStringLiteral("A")},
        {QStringLiteral("Cross"), QStringLiteral("Z")},
        {QStringLiteral("Circle"), QStringLiteral("X")},
        {QStringLiteral("Triangle"), QStringLiteral("S")},
        {QStringLiteral("DpadLeft"), QStringLiteral("NumPad4")},
        {QStringLiteral("DpadDown"), QStringLiteral("NumPad5")},
        {QStringLiteral("DpadRight"), QStringLiteral("NumPad6")},
        {QStringLiteral("DpadUp"), QStringLiteral("NumPad8")},
        {QStringLiteral("R1"), QStringLiteral("R")},
        {QStringLiteral("R2"), QStringLiteral("RControlKey")},
        {QStringLiteral("R3"), QStringLiteral("LControlKey")},
        {QStringLiteral("L1"), QStringLiteral("L")},
        {QStringLiteral("L2"), QStringLiteral("RShiftKey")},
        {QStringLiteral("L3"), QStringLiteral("LShiftKey")},
    };
}

Settings::Settings() {
#ifdef Q_OS_WIN
    m_path = QDir::homePath() + QStringLiteral("/AppData/Roaming/PPSX33");
#else
    m_path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (m_path.isEmpty())
        m_path = QDir::homePath() + QStringLiteral("/.config/PPSX33");
#endif
    QDir().mkpath(m_path);
    m_path = QDir(m_path).filePath(QStringLiteral("settings.json"));
    m_keys = defaultKeyMap();
    m_threads = qMax(1, QThread::idealThreadCount());
    load();
}

void Settings::load() {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    const QJsonObject o = doc.object();
    m_gfx = o.value(QStringLiteral("GraphicsBackend")).toString(m_gfx);
    if (m_gfx.isEmpty()) m_gfx = QStringLiteral("Vulkan");
    m_threads = o.value(QStringLiteral("PpuThreads")).toInt(m_threads);
    if (m_threads < 1) m_threads = qMax(1, QThread::idealThreadCount());
    m_lastElf = o.value(QStringLiteral("LastElfPath")).toString();
    m_lastBin = o.value(QStringLiteral("LastBinPath")).toString();
    m_lastRoot = o.value(QStringLiteral("LastGamesRootPath")).toString();
    m_lastGame = o.value(QStringLiteral("LastGameFolderPath")).toString();
    m_lastOut = o.value(QStringLiteral("LastOutputFolderPath")).toString();
    m_termsOk = o.value(QStringLiteral("TermsAccepted")).toBool(false);
    m_termsBy = o.value(QStringLiteral("TermsAcceptedBy")).toString();
    m_termsVer = o.value(QStringLiteral("TermsVersion")).toString();
    const QJsonObject km = o.value(QStringLiteral("KeyMap")).toObject();
    if (!km.isEmpty()) {
        m_keys = defaultKeyMap();
        for (auto it = km.begin(); it != km.end(); ++it)
            m_keys[it.key()] = it.value().toString();
    }
}

void Settings::save() const {
    QJsonObject o;
    o.insert(QStringLiteral("GraphicsBackend"), m_gfx);
    o.insert(QStringLiteral("PpuThreads"), m_threads);
    o.insert(QStringLiteral("LastElfPath"), m_lastElf);
    o.insert(QStringLiteral("LastBinPath"), m_lastBin);
    o.insert(QStringLiteral("LastGamesRootPath"), m_lastRoot);
    o.insert(QStringLiteral("LastGameFolderPath"), m_lastGame);
    o.insert(QStringLiteral("LastOutputFolderPath"), m_lastOut);
    QJsonObject km;
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it)
        km.insert(it.key(), it.value());
    o.insert(QStringLiteral("KeyMap"), km);
    o.insert(QStringLiteral("TermsAccepted"), m_termsOk);
    o.insert(QStringLiteral("TermsAcceptedBy"), m_termsBy);
    o.insert(QStringLiteral("TermsVersion"), m_termsVer);
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}

void Settings::setTermsAccepted(bool v, const QString& by, const QString& ver) {
    m_termsOk = v;
    m_termsBy = by;
    m_termsVer = ver;
}
