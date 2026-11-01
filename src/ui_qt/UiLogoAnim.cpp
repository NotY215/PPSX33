#include "UiLogoAnim.h"
#include <QLabel>
#include <QTimer>
#include <QPixmap>
#include <QCoreApplication>
#include <QtMath>

UiLogoAnim::UiLogoAnim(QLabel* logo, QObject* parent) : QObject(parent), m_logo(logo) {
    m_path = QCoreApplication::applicationDirPath() + QStringLiteral("/ppsx33-logo.png");
    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &UiLogoAnim::tick);
}

void UiLogoAnim::start() { if (m_timer) m_timer->start(); }
void UiLogoAnim::stop() { if (m_timer) m_timer->stop(); }

void UiLogoAnim::tick() {
    if (!m_logo) return;
    m_phase += 0.04;
    const qreal pulse = 0.5 + 0.5 * qSin(m_phase);
    const qreal scale = 1.0 + 0.04 * pulse;
    QPixmap base(m_path);
    if (base.isNull()) base = QPixmap(QStringLiteral(":/assets/ppsx33-logo.png"));
    if (base.isNull()) return;
    const int side = int(100 * scale);
    m_logo->setPixmap(base.scaled(side, side, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logo->setStyleSheet(QStringLiteral("opacity: %1;").arg(0.85 + 0.15 * pulse));
}
