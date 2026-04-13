#include "CenterPanel.h"

static const QColor C_BG     = QColor(8,   5,   2);
static const QColor C_TRACK  = QColor(35,  20,  6);
static const QColor C_ORANGE = QColor(180, 95,  15);
static const QColor C_WHITE  = QColor(240, 235, 225);
static const QColor C_DIM    = QColor(130, 90,  40);
static const QColor C_MUTED  = QColor(45,  28,  8);
static const QColor C_GREEN  = QColor(32,  200, 80);
static const QColor C_RED    = QColor(220, 50,  50);
static const QColor C_AMBER  = QColor(220, 180, 40);
static const QColor C_BLINK  = QColor(240, 160, 0);   // blinker orange

CenterPanel::CenterPanel(QWidget *parent) : QWidget(parent) {
}

// Draw a filled arrow pointing left or right
static void drawArrow(QPainter &p, double cx, double cy,
                      double w, double h, bool leftArrow, QColor color)
{
    QPainterPath arrow;
    if (leftArrow) {
        // Left-pointing arrow
        arrow.moveTo(cx,          cy);           // tip
        arrow.lineTo(cx + w*0.45, cy - h*0.50); // top-right
        arrow.lineTo(cx + w*0.45, cy - h*0.22); // notch top
        arrow.lineTo(cx + w,      cy - h*0.22); // tail top
        arrow.lineTo(cx + w,      cy + h*0.22); // tail bottom
        arrow.lineTo(cx + w*0.45, cy + h*0.22); // notch bottom
        arrow.lineTo(cx + w*0.45, cy + h*0.50); // bottom-right
        arrow.closeSubpath();
    } else {
        // Right-pointing arrow
        arrow.moveTo(cx + w,      cy);           // tip
        arrow.lineTo(cx + w*0.55, cy - h*0.50); // top-left
        arrow.lineTo(cx + w*0.55, cy - h*0.22); // notch top
        arrow.lineTo(cx,          cy - h*0.22); // tail top
        arrow.lineTo(cx,          cy + h*0.22); // tail bottom
        arrow.lineTo(cx + w*0.55, cy + h*0.22); // notch bottom
        arrow.lineTo(cx + w*0.55, cy + h*0.50); // bottom-left
        arrow.closeSubpath();
    }
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawPath(arrow);
}

void CenterPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const double W  = width();
    const double H  = height();
    const double cx = W / 2.0;

    p.fillRect(rect(), C_BG);

    // ─────────────────────────────────────────────────────────────────────
    // Row 0   0%  –  11%   time + temp
    // Row 1  11%  –  21%   steering bar
    // Row 2  21%  –  44%   blinker arrows
    // Row 3  44%  –  62%   TEMP + FUEL bars
    // Row 4  62%  –  80%   warning icons
    // Row 5  80%  –  91%   light icons
    // Row 6  91%  – 100%   CAN pill + odometer
    // ─────────────────────────────────────────────────────────────────────

    // ── Row 0: Time | Temp ────────────────────────────────────────────────
    {
        double rY = H * 0.01;
        double rH = H * 0.10;
        int    fs = qBound(13, qRound(rH * 0.60), 32);

        p.setPen(C_WHITE);
        p.setFont(QFont(appFont(), fs, QFont::Bold));
        p.drawText(QRectF(0, rY, W * 0.52, rH),
                   Qt::AlignLeft | Qt::AlignVCenter, timeStr);

        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), fs));
        p.drawText(QRectF(W * 0.48, rY, W * 0.52, rH),
                   Qt::AlignRight | Qt::AlignVCenter, tempStr);
    }

    // ── Row 1: Steering bar ───────────────────────────────────────────────
    {
        double rY   = H * 0.12;
        double bH   = qBound(5.0, H * 0.018, 12.0);
        double bW   = W * 0.80;
        double bX   = cx - bW / 2.0;
        int    lblFs = qBound(7, qRound(bH * 1.3), 13);

        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), lblFs));
        p.drawText(QRectF(bX - 28, rY, 26, bH + 2),
                   Qt::AlignRight | Qt::AlignVCenter, "0");
        p.drawText(QRectF(bX + bW + 2, rY, 32, bH + 2),
                   Qt::AlignLeft  | Qt::AlignVCenter, "160");

        p.setBrush(C_TRACK);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(bX, rY, bW, bH), 3, 3);

        double norm  = (steerAngle + 160.0) / 320.0;
        double fillW = qAbs(norm - 0.5) * bW;
        double fillX = norm < 0.5 ? (cx - fillW) : cx;
        p.setBrush(C_ORANGE);
        p.drawRoundedRect(QRectF(fillX, rY, fillW, bH), 3, 3);

        p.setBrush(C_WHITE);
        p.drawEllipse(QPointF(cx, rY + bH / 2.0), 3, 3);

        int angFs = qBound(7, qRound(bH * 1.1), 12);
        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), angFs));
        p.drawText(QRectF(cx - 26, rY + bH + 3, 52, angFs + 4),
                   Qt::AlignCenter,
                   QString("%1°").arg(qRound(steerAngle)));

        // --- DRAW PEDALS ---
        double pedY = rY + bH + 22; 
        p.setFont(QFont(appFont(), 8, QFont::Bold));
        
        // Brake Indicator (Left Side)
        if (brakePressed) {
            p.setBrush(C_RED); p.setPen(Qt::NoPen);
            p.drawRoundedRect(cx - 50, pedY, 40, 14, 3, 3);
            p.setPen(Qt::white);
            p.drawText(QRectF(cx - 50, pedY, 40, 14), Qt::AlignCenter, "BRK");
        }
        
        // Accelerator Gauge (Right Side)
        double accW = 40.0 * (accelerator / 255.0);
        p.setBrush(C_TRACK); p.setPen(Qt::NoPen);
        p.drawRoundedRect(cx + 10, pedY, 40, 14, 3, 3);
        p.setBrush(C_GREEN);
        p.drawRoundedRect(cx + 10, pedY, accW, 14, 3, 3);
        p.setPen(C_DIM);
        p.drawText(QRectF(cx + 10, pedY + 16, 40, 12), Qt::AlignCenter, "ACC");
    }

    // ── Row 2: Blinker arrows ─────────────────────────────────────────────
    {
        double rY   = H * 0.22;
        double rH   = H * 0.20;
        double arW  = W * 0.28;
        double arH  = rH * 0.55;

        // Left arrow — left quarter of panel
        double lAX  = cx - W * 0.42;
        double lAY  = rY + rH * 0.22;

        // Right arrow — right quarter of panel
        double rAX  = cx + W * 0.14;
        double rAY  = rY + rH * 0.22;

        bool showLeft  = (blinkerStatus == 1 || blinkerStatus == 3);
        bool showRight = (blinkerStatus == 2 || blinkerStatus == 3);

        // Left arrow
        QColor leftCol  = showLeft ? C_BLINK : QColor(45, 28, 8, 80);
        drawArrow(p, lAX, lAY, arW, arH, true, leftCol);

        if (!showLeft) {
            p.setPen(QPen(QColor(60, 38, 10, 100), 1));
            p.setBrush(Qt::NoBrush);
        }

        // Right arrow
        QColor rightCol = showRight ? C_BLINK : QColor(45, 28, 8, 80);
        drawArrow(p, rAX, rAY, arW, arH, false, rightCol);

        // Hazard indicator
        if (blinkerStatus == 3) {
            p.setPen(C_BLINK);
            p.setFont(QFont(appFont(), qBound(9, qRound(rH * 0.18), 16), QFont::Bold));
            p.drawText(QRectF(cx - 30, rY + rH * 0.35, 60, rH * 0.3), Qt::AlignCenter, "HZD");
        }

        // Wiper status
        if (wiperStatus > 0) {
            QString wiperTxt = wiperStatus == 2 ? "WIPER HI" : "WIPER LO";
            p.setPen(C_DIM);
            p.setFont(QFont(appFont(), qBound(7, qRound(rH * 0.14), 12)));
            p.drawText(QRectF(cx - 40, rY + rH * 0.78, 80, rH * 0.2), Qt::AlignCenter, wiperTxt);
        }
    }

    // ── Row 3: TEMP + FUEL bars ───────────────────────────────────────────
    {
        double rY   = H * 0.44;
        double rH   = H * 0.17;
        double bW   = W * 0.36;
        double bH   = qBound(8.0, rH * 0.18, 16.0);
        double lX   = cx - W * 0.02 - bW;
        double rX   = cx + W * 0.02;

        int lblFs = qBound(9,  qRound(rH * 0.20), 16);
        int valFs = qBound(13, qRound(rH * 0.32), 28);

        auto drawBar = [&](double x, double pct,
                           QColor fill, QString label, QString valStr) {
            p.setPen(C_DIM);
            p.setFont(QFont(appFont(), lblFs));
            double lblH = lblFs + 6;
            p.drawText(QRectF(x, rY, bW, lblH),
                       Qt::AlignCenter, label);

            double barY = rY + lblH + 2;
            p.setBrush(C_TRACK);
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(QRectF(x, barY, bW, bH), bH/2, bH/2);

            double fw = qBound(0.0, pct, 1.0) * bW;
            p.setBrush(fill);
            p.drawRoundedRect(QRectF(x, barY, fw, bH), bH/2, bH/2);

            double valY = barY + bH + 4;
            p.setPen(C_WHITE);
            p.setFont(QFont(appFont(), valFs, QFont::Bold));
            p.drawText(QRectF(x, valY, bW, valFs + 6),
                       Qt::AlignCenter, valStr);
        };

        double tempPct = qBound(0.0, coolant / 120.0, 1.0);
        QColor tempCol = coolant > 100 ? C_RED : QColor(160, 85, 12);
        double fuelPct = qBound(0.0, m_fuel, 1.0);
        QColor fuelCol = fuelPct < 0.2 ? C_RED : QColor(140, 70, 8);

        drawBar(lX, tempPct, tempCol,
                "TEMP", QString("%1°C").arg(qRound(coolant)));
        drawBar(rX, fuelPct, fuelCol,
                "FUEL", QString("%1%").arg(qRound(fuelPct * 100)));

        // --- DRAW DOORS (Center of the Bars) ---
        double carW = 16;
        double carH = 34;
        
        // Car Body Outline
        p.setPen(QPen(C_DIM, 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(cx - carW/2, rY + 6, carW, carH, 4, 4);
        
        // Driver Door (Left Side Open)
        if (doorDriverOpen) { 
            p.setBrush(C_RED);
            p.setPen(Qt::NoPen);
            p.drawRect(cx - carW/2 - 6, rY + 10, 6, 14);
        }
        
        // Passenger Door (Right Side Open)
        if (doorPassengerOpen) {
            p.setBrush(C_RED);
            p.setPen(Qt::NoPen);
            p.drawRect(cx + carW/2, rY + 10, 6, 14);
        }
    }

    // ── Row 4: Warning icons ──────────────────────────────────────────────
    {
        double rY   = H * 0.62;
        double rH   = H * 0.17;
        double icoD = qBound(22.0, rH * 0.62, 46.0);
        double icoR = icoD / 2.0;
        double icoY = rY + rH * 0.42;
        int    symFs = qBound(9, qRound(icoD * 0.42), 18);
        int    capFs = qBound(5, qRound(icoD * 0.22), 9);

        // Dynamic light status (1 = MINIMOS/LOW, 2 = MEDIOS/MEDIUM)
        bool isLightOn = (lightMode > 0);
        QColor currentLightCol = (lightMode == 2) ? C_ORANGE : C_GREEN;
        QString currentLightCap = (lightMode == 2) ? "MED" : "LOW";

        struct Icon { QString sym; QString cap; bool on; QColor col; };
        QVector<Icon> icons = {
            { "⚙", "SYS",  false,    C_MUTED  },
            { "⚠", "OIL",  warnOil,  C_AMBER  },
            { "⚠", "ABS",  warnABS,  C_AMBER  },
            { "▣", "ENG",  warnEng,  C_ORANGE },
            { "⛽", "FUEL", warnFuel, C_ORANGE },
            { "⚠", "BAT",  warnBat,  C_AMBER  },
            { "◉", currentLightCap, isLightOn, currentLightCol }, 
            { "◑", "HIGH", lightHigh, QColor(60, 120, 240) },
            { "◎", "FOG",  lightFog,  C_ORANGE },
        };

        double spacing = W / (double)(icons.size() + 1);

        for (int i = 0; i < icons.size(); i++) {
            double ix  = spacing * (i + 1);
            bool   on  = icons[i].on;
            QColor col = on ? icons[i].col : C_MUTED;

            if (on) {
                p.setBrush(icons[i].col.darker(260));
                p.setPen(QPen(icons[i].col, 1.5));
            } else {
                p.setBrush(QColor(18, 20, 28));
                p.setPen(QPen(C_MUTED, 1.0));
            }
            p.drawEllipse(QPointF(ix, icoY), icoR, icoR);

            p.setPen(col);
            p.setFont(QFont(appFont(), symFs));
            p.drawText(QRectF(ix - icoR, icoY - icoR, icoD, icoD * 0.65), Qt::AlignCenter, icons[i].sym);

            p.setFont(QFont(appFont(), capFs, QFont::Bold));
            p.drawText(QRectF(ix - icoR, icoY + icoR * 0.05, icoD, icoR * 0.90), Qt::AlignCenter, icons[i].cap);
        }
    }

    // ── Row 5: CAN pill + odometer ────────────────────────────────────────
    {
        double rY    = H * 0.82;
        double rH    = H * 0.18;
        double pillH = qBound(16.0, rH * 0.36, 24.0);
        double pillW = W * 0.52;
        int    pFs   = qBound(8, qRound(pillH * 0.50), 13);

        QColor canFg = canOnline ? C_GREEN : C_RED;
        QColor canBg = canOnline ? QColor(8,24,8) : QColor(24,8,8);

        QRectF canBox(cx - pillW/2.0, rY, pillW, pillH);
        QPainterPath pill;
        pill.addRoundedRect(canBox, 5, 5);
        p.fillPath(pill, canBg);
        p.setPen(QPen(canFg, 1));
        p.drawPath(pill);
        p.setPen(canFg);
        p.setFont(QFont(appFont(), pFs));
        p.drawText(canBox, Qt::AlignCenter, canStatus);

        int odoFs = qBound(11, qRound(rH * 0.32), 18);
        p.setPen(C_DIM);
        p.setFont(QFont(appFont(), odoFs, QFont::Bold));
        p.drawText(QRectF(cx - 110, rY + pillH + 4, 220, odoFs + 6),
                   Qt::AlignCenter,
                   QString("%1 km").arg(odometer));
    }
}