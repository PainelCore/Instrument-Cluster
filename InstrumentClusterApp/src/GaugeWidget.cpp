#include "GaugeWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QTimer>
#include <QResizeEvent>
#include "AppFont.h"

static const QColor C_BG     = QColor(8,   5,   2);
static const QColor C_CARD   = QColor(20,  12,  4);
static const QColor C_BORDER = QColor(50,  28,  8);
static const QColor C_WHITE  = QColor(240, 235, 225);
static const QColor C_DIM    = QColor(130,  90, 40);

// Helper to construct the rounded rectangle track
static QPainterPath buildPerim(QRectF r, double cr) {
    QPainterPath path;
    path.moveTo(r.left() + cr, r.top());
    path.lineTo(r.right() - cr, r.top());
    path.arcTo(QRectF(r.right()-2*cr, r.top(),        2*cr, 2*cr),  90, -90);
    path.lineTo(r.right(), r.bottom() - cr);
    path.arcTo(QRectF(r.right()-2*cr, r.bottom()-2*cr, 2*cr, 2*cr),  0, -90);
    path.lineTo(r.left() + cr, r.bottom());
    path.arcTo(QRectF(r.left(),       r.bottom()-2*cr, 2*cr, 2*cr), 270, -90);
    path.lineTo(r.left(), r.top() + cr);
    path.arcTo(QRectF(r.left(),       r.top(),         2*cr, 2*cr), 180, -90);
    path.closeSubpath();
    return path;
}

GaugeWidget::GaugeWidget(Config cfg, QWidget *parent)
    : QWidget(parent), m_cfg(cfg), m_value(0), m_displayValue(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent);

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        double diff = m_value - m_displayValue;
        if (qAbs(diff) < 0.5) {
            m_displayValue = m_value;
            m_animTimer->stop();
        } else {
            double ease = 0.05; 
            if (qAbs(diff) > 500) ease = 0.35;      // Downshift / Rev - Very Fast
            else if (qAbs(diff) > 50) ease = 0.15;  // Acceleration - Medium
            
            m_displayValue += diff * ease;
        }
        update();
    });
}

void GaugeWidget::setValue(double v) {
    m_value = qBound(m_cfg.minVal, v, m_cfg.maxVal);
    if (!m_animTimer->isActive())
        m_animTimer->start(33);  // around 30fps 
}

void GaugeWidget::resizeEvent(QResizeEvent *event) {
    // Flag the cache to be rebuilt whenever the window size changes (desktop)
    m_cacheDirty = true;
    QWidget::resizeEvent(event);
}

//
// OPTIMIZATION BY A LLM - Caches all the static geometry into a QPixmap to avoid expensive calculations on every frame
//

void GaugeWidget::rebuildCache() {
    const double W  = width();
    const double H  = height();
    if (W <= 0 || H <= 0) return;

    const double cx = W / 2.0;
    const double cy = H / 2.0;
    const double S  = qMin(W, H);

    // Initialize an empty transparent cache image
    m_bgCache = QPixmap(W, H);
    m_bgCache.fill(Qt::transparent);

    QPainter p(&m_bgCache);
    p.setRenderHint(QPainter::Antialiasing);

    // ── 1. Geometry Math ──
    double cardPad    = S * 0.04;
    double cardRadius = S * 0.13;
    QRectF cardRect(cx - S/2 + cardPad, cy - S/2 + cardPad, S - cardPad*2, S - cardPad*2);

    double outerPad = S * 0.045;
    double innerPad = S * 0.145;
    double bandCr   = cardRadius * 0.85;

    QRectF outerRect(cx - S/2 + outerPad, cy - S/2 + outerPad, S - outerPad*2, S - outerPad*2);
    QRectF innerRect(cx - S/2 + innerPad, cy - S/2 + innerPad, S - innerPad*2, S - innerPad*2);

    QPainterPath outerPerim = buildPerim(outerRect, bandCr);
    QPainterPath innerPerim = buildPerim(innerRect, bandCr);

    double startPct = m_cfg.arcStartPct;
    double spanPct  = m_cfg.arcSpanPct;

    // ── 2. Draw Outer Card ──
    QPainterPath cardPath;
    cardPath.addRoundedRect(cardRect, cardRadius, cardRadius);
    p.fillPath(cardPath, C_CARD);
    p.setPen(QPen(C_BORDER, 1.2));
    p.drawPath(cardPath);

    // ── 3. Pre-calculate active band points ──
    // This turns evaluating curves at runtime into instantly connecting pre-computed dots
    m_outerPts.clear();
    m_innerPts.clear();
    
    double totalLen = outerPerim.length();
    int numSteps = qMax(100, qRound(totalLen * spanPct / 1.5));
    
    for (int i = 0; i <= numSteps; i++) {
        double pct = startPct + (spanPct * i) / numSteps;
        while (pct > 1.0) pct -= 1.0; // handle wrap around
        m_outerPts.append(outerPerim.pointAtPercent(pct));
        m_innerPts.append(innerPerim.pointAtPercent(pct));
    }

    // Draw the static dark track band using our newly calculated points
    if (!m_outerPts.isEmpty()) {
        QPainterPath trackBand;
        trackBand.moveTo(m_outerPts.first());
        for (int i = 1; i <= numSteps; i++) trackBand.lineTo(m_outerPts[i]);
        for (int i = numSteps; i >= 0; i--) trackBand.lineTo(m_innerPts[i]);
        trackBand.closeSubpath();
        p.fillPath(trackBand, QColor(25, 15, 5));
    }

    // ── 4. Draw Inner Card ──
    double innerCardPad    = S * 0.150;
    double innerCardRadius = cardRadius * 0.80;
    QRectF innerCardRect(cx - S/2 + innerCardPad, cy - S/2 + innerCardPad, S - innerCardPad*2, S - innerCardPad*2);
    QPainterPath innerCardPath;
    innerCardPath.addRoundedRect(innerCardRect, innerCardRadius, innerCardRadius);
    p.fillPath(innerCardPath, QColor(16, 9, 3));
    p.setPen(QPen(C_BORDER, 0.8));
    p.drawPath(innerCardPath);

    // ── 5. Layout and Draw Dim Ticks ──
    m_ticks.clear();
    double step = m_cfg.tickStep > 0 ? m_cfg.tickStep : (m_cfg.maxVal - m_cfg.minVal) / 4.0;
    int numTicks = qRound((m_cfg.maxVal - m_cfg.minVal) / step);
    int lblFs = qBound(6, qRound(S * 0.028), 12);
    p.setFont(QFont(appFont(), lblFs));

    for (int i = 0; i <= numTicks; i++) {
        double pct = (double)i / (double)numTicks;
        double perimPct = startPct + pct * spanPct;
        while (perimPct > 1.0) perimPct -= 1.0;

        QPointF onArc = outerPerim.pointAtPercent(perimPct);
        QPointF toCenter(cx - onArc.x(), cy - onArc.y());
        double dist = qSqrt(toCenter.x()*toCenter.x() + toCenter.y()*toCenter.y());
        
        if (dist > 1.0) {
            QPointF unit(toCenter.x()/dist, toCenter.y()/dist);
            double inset = (innerPad - outerPad) * 0.52 + lblFs;
            double lx = onArc.x() + unit.x() * inset;
            double ly = onArc.y() + unit.y() * inset;
            
            double val = m_cfg.minVal + pct * (m_cfg.maxVal - m_cfg.minVal);
            TickData td;
            td.text = QString::number(qRound(val));
            td.rect = QRectF(lx - lblFs*3, ly - lblFs, lblFs*6, lblFs*2);
            td.valuePct = pct;
            m_ticks.append(td);

            // Draw the inactive/dim text directly onto the cache
            p.setPen(C_DIM);
            p.drawText(td.rect, Qt::AlignCenter, td.text);
        }
    }

    // ── 6. Draw Unit Label ──
    double icW = S - innerCardPad * 2;
    int unitFs = qBound(8, qRound(icW * 0.075), 20);
    p.setPen(C_DIM);
    p.setFont(QFont(appFont(), unitFs));
    p.drawText(QRectF(cx - icW/2.0, cy + icW*0.18, icW, icW*0.16), Qt::AlignCenter, m_cfg.unit);

    // ── 7. Pre-calculate layout for main dynamic value ──
    m_valFs = qBound(16, qRound(icW * 0.22), 72);
    m_shadowValueRect = QRectF(cx - icW/2.0 + 2, cy - icW*0.24 + 2, icW, icW*0.48);
    m_mainValueRect = QRectF(cx - icW/2.0, cy - icW*0.24, icW, icW*0.48);
}

void GaugeWidget::paintEvent(QPaintEvent *) {
    // Check if window resized or just booted up
    if (m_cacheDirty) {
        rebuildCache();
        m_cacheDirty = false;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), C_BG);
    
    p.drawPixmap(0, 0, m_bgCache);

    double valuePct = (m_displayValue - m_cfg.minVal) / (m_cfg.maxVal - m_cfg.minVal);
    valuePct = qBound(0.0, valuePct, 1.0);

    if (valuePct > 0.001 && !m_outerPts.isEmpty()) {
        int ptCount = qRound(valuePct * (m_outerPts.size() - 1));
        if (ptCount > 0) {
            QPainterPath activeBand;
            activeBand.moveTo(m_outerPts[0]);
            for (int i = 1; i <= ptCount; i++) activeBand.lineTo(m_outerPts[i]);
            for (int i = ptCount; i >= 0; i--) activeBand.lineTo(m_innerPts[i]);
            activeBand.closeSubpath();

            QLinearGradient grad(m_outerPts.first(), m_outerPts[ptCount]);
            grad.setColorAt(0.0, QColor(30,  14,  3, 255));
            grad.setColorAt(0.3, QColor(80,  35,  5, 255));
            grad.setColorAt(0.7, QColor(150, 68,  8, 255));
            grad.setColorAt(1.0, QColor(190, 95, 12, 255));

            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawPath(activeBand);
        }
    }

    int lblFs = qBound(6, qRound(qMin(width(), height()) * 0.028), 12);
    p.setFont(QFont(appFont(), lblFs, QFont::Bold));
    
    for (const TickData &td : m_ticks) {
        if (td.valuePct <= valuePct) {
            double brightness = qBound(0.0, td.valuePct / qMax(valuePct, 0.001), 1.0);
            
            // Drop Shadow
            p.setPen(QColor(0, 0, 0, 160));
            p.drawText(td.rect.translated(1, 1), Qt::AlignCenter, td.text);

            // Bright Text
            int cr = qRound(190 + brightness * 65);
            int cg = qRound(140 + brightness * 95);
            int cb = qRound( 80 + brightness * 145);
            p.setPen(QColor(cr, cg, cb, 255));
            p.drawText(td.rect, Qt::AlignCenter, td.text);
        }
    }

    // Main Value Text
    int displayNum = qRound(m_displayValue);
    
    // Smooth the text output: Round RPM to nearest 10 to stop micro-flicker
    if (m_cfg.unit == "RPM") {
        displayNum = qRound(m_displayValue / 10.0) * 10;
    }
    
    QString valText = QString::number(displayNum);
    p.setFont(QFont(appFont(), m_valFs, QFont::Bold));
    
    p.setPen(QColor(0, 0, 0, 160)); // Shadow
    p.drawText(m_shadowValueRect, Qt::AlignCenter, valText);
    
    p.setPen(C_WHITE); // Main
    p.drawText(m_mainValueRect, Qt::AlignCenter, valText);
}