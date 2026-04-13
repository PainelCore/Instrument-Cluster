#include "PillBadge.h"
#include <QPainter>
#include <QPainterPath>
#include "AppFont.h"

namespace Style {
    extern const QColor CARD_BORDER;
    extern const QColor TEXT_WHITE;
    extern const QColor TEXT_DIM;
}

PillBadge::PillBadge(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(28);
}

void PillBadge::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = QRectF(rect()).adjusted(0, 2, 0, -2);
    QPainterPath pill;
    pill.addRoundedRect(r, 12, 12);
    p.fillPath(pill, QColor(22, 25, 35));
    p.setPen(QPen(QColor(38, 42, 55), 1));
    p.drawPath(pill);

    double third = r.width() / 3.0;

    p.setPen(QColor(90, 95, 120));
    p.setFont(QFont(appFont(), 10));
    p.drawText(QRectF(r.x(),           r.y(), third, r.height()),
               Qt::AlignCenter, leftLabel);

    p.setPen(QColor(240, 242, 248));
    p.setFont(QFont(appFont(), 10, QFont::Bold));
    p.drawText(QRectF(r.x() + third,   r.y(), third, r.height()),
               Qt::AlignCenter, midValue);

    p.setPen(QColor(90, 95, 120));
    p.setFont(QFont(appFont(), 10));
    p.drawText(QRectF(r.x() + 2*third, r.y(), third, r.height()),
               Qt::AlignCenter, rightValue);
}