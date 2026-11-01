#pragma once
#include <QObject>
class QLabel;
class QTimer;

class UiLogoAnim : public QObject {
    Q_OBJECT
public:
    explicit UiLogoAnim(QLabel* logo, QObject* parent = nullptr);
    void start();
    void stop();
private slots:
    void tick();
private:
    QLabel* m_logo = nullptr;
    QTimer* m_timer = nullptr;
    qreal m_phase = 0;
    QString m_path;
};
