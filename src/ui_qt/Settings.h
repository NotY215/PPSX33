#pragma once
#include <QString>
#include <QMap>
#include <QJsonObject>

class Settings {
public:
    static Settings& instance();
    void load();
    void save() const;

    QString graphicsBackend() const { return m_gfx; }
    void setGraphicsBackend(const QString& v) { m_gfx = v; }

    int ppuThreads() const { return m_threads; }
    void setPpuThreads(int v) { m_threads = v; }

    QString lastElfPath() const { return m_lastElf; }
    void setLastElfPath(const QString& v) { m_lastElf = v; }

    QString lastGameFolderPath() const { return m_lastGame; }
    void setLastGameFolderPath(const QString& v) { m_lastGame = v; }

    QString lastGamesRootPath() const { return m_lastRoot; }
    void setLastGamesRootPath(const QString& v) { m_lastRoot = v; }

    QString lastOutputFolderPath() const { return m_lastOut; }
    void setLastOutputFolderPath(const QString& v) { m_lastOut = v; }

    QMap<QString, QString> keyMap() const { return m_keys; }
    void setKeyMap(const QMap<QString, QString>& k) { m_keys = k; }
    void setKey(const QString& action, const QString& key) { m_keys[action] = key; }

    bool termsAccepted() const { return m_termsOk; }
    void setTermsAccepted(bool v, const QString& by, const QString& ver);

    QString path() const { return m_path; }

    static QMap<QString, QString> defaultKeyMap();

private:
    Settings();
    QString m_path;
    QString m_gfx = QStringLiteral("Vulkan");
    int m_threads = 4;
    QString m_lastElf, m_lastBin, m_lastRoot, m_lastGame, m_lastOut;
    QMap<QString, QString> m_keys;
    bool m_termsOk = false;
    QString m_termsBy, m_termsVer;
};
