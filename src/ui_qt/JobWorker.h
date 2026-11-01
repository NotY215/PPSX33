#pragma once
#include <QObject>
#include <QString>

class JobWorker : public QObject {
    Q_OBJECT
public slots:
    void lift(const QString& projectDir);
    void build(const QString& projectDir, const QString& distDir);
signals:
    void finished(bool ok, QString log, QString kind);
};
