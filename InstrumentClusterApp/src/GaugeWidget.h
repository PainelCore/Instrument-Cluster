#pragma once
#include <QWidget>
#include <QColor>
#include <QString>
#include <QTimer>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QRectF>

class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    struct Config {
        QString unit;
        double  minVal       = 0;
        double  maxVal       = 7000;
        double  tickStep     = 1000;
        double  arcStartPct  = 0.62;
        double  arcSpanPct   = 0.76;
        QColor  arcActiveColor;
    };

    explicit GaugeWidget(Config cfg, QWidget *parent = nullptr);
    void   setValue(double v);
    double value() const { return m_value; }
    double displayValue() const { return m_displayValue; }

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Config  m_cfg;
    double  m_value        = 0;
    double  m_displayValue = 0;  
    QTimer *m_animTimer    = nullptr;

    // Caching
    QPixmap m_bgCache;
    bool m_cacheDirty = true;
    
    // Pre-calculated vector points so we aren't doing math in the render loop
    QVector<QPointF> m_outerPts;
    QVector<QPointF> m_innerPts;
    
    // Pre-calculated layout positions for the ticks
    struct TickData {
        QString text;
        QRectF rect;
        double valuePct;
    };
    QVector<TickData> m_ticks;
    
    QRectF m_mainValueRect;
    QRectF m_shadowValueRect;
    int m_valFs;
    
    void rebuildCache();
};