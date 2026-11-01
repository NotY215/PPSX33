#pragma once
#include <QWidget>
class QLabel;
class QTimer;

class UiLogoAnim : public QWidget {
    Q_OBJECT
public:
    explicit UiLogoAnim(QWidget* parent = nullptr);
    void setLogoPath(const QString& path);
    void start();
    void stop();
protected:
    void paintEvent(QPaintEvent*) override;
    QSize sizeHint() const override { return QSize(120, 120); }
private slots:
    void tick();
private:
    QString m_path;
    QTimer* m_timer = nullptr;
    qreal m_angle = 0;
    qreal m_pulse = 0;
};
