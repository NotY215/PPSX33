#pragma once
#include <QString>
#include <functional>

class NativeBridge {
public:
    static NativeBridge& instance();
    bool load(const QString& dir = {});
    bool ok() const { return m_ok; }
    QString lastError() const { return m_err; }

    int version() const;
    bool createProject(const QString& root, const QString& game, const QString& elf,
                       QString* projectDir, QString* err);
    bool liftProject(const QString& projectDir, QString* log);
    void beginBuild();
    void cancelBuild();
    bool buildProject(const QString& projectDir, const QString& runtimeDir,
                      const QString& compilersDir, QString* log);

private:
    NativeBridge() = default;
    bool m_ok = false;
    QString m_err;
    void* m_lib = nullptr;
    int (*m_version)() = nullptr;
    int (*m_create)(const char*, const char*, const char*, char*, int, char*, int) = nullptr;
    int (*m_lift)(const char*, char*, int) = nullptr;
    void (*m_begin)() = nullptr;
    void (*m_cancel)() = nullptr;
    int (*m_build)(const char*, const char*, const char*, char*, int) = nullptr;
};
