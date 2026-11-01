#include "UiLogoAnim.h"
#include <QPainter>
#include <QTimer>
#include <QPixmap>
#include <QtMath>
#include <QPaintEvent>

UiLogoAnim::UiLogoAnim(QWidget* parent) : QWidget(parent) {
    setFixedSize(120, 120);
    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &UiLogoAnim::tick);
}

void UiLogoAnim::setLogoPath(const QString& path) { m_path = path; update(); }
void UiLogoAnim::start() { if (m_timer) m_timer->start(); }
void UiLogoAnim::stop() { if (m_timer) m_timer->stop(); }

void UiLogoAnim::tick() {
    m_angle += 3.5;
    if (m_angle >= 360.0) m_angle -= 360.0;
    m_pulse += 0.06;
    update();
}

void UiLogoAnim::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRectF r = rect().adjusted(6, 6, -6, -6);
    const QPointF c = r.center();

    // Dashed rotating ring (C# / glass-UI style)
    QPen ring(QColor(0, 229, 255, 160));
    ring.setWidthF(2.4);
    ring.setStyle(Qt::DashLine);
    ring.setDashPattern({4, 3});
    p.save();
    p.translate(c);
    p.rotate(m_angle);
    p.translate(-c);
    p.setPen(ring);
    p.drawEllipse(r);
    p.restore();

    // Soft outer glow arc
    QPen arc(QColor(59, 130, 246, 120));
    arc.setWidthF(3.0);
    arc.setCapStyle(Qt::RoundCap);
    p.setPen(arc);
    p.drawArc(r, int((-m_angle) * 16), 90 * 16);

    // Logo in center with slight pulse
    QPixmap pm(m_path);
    if (pm.isNull())
        pm = QPixmap(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (!pm.isNull()) {
        const qreal s = 1.0 + 0.03 * qSin(m_pulse);
        const int side = int(72 * s);
        QPixmap scaled = pm.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint tl(int(c.x() - scaled.width() / 2.0), int(c.y() - scaled.height() / 2.0));
        p.drawPixmap(tl, scaled);
    }
}
