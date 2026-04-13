#pragma once
#include <QWidget>
#include <QString>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include "AppFont.h"

class CenterPanel : public QWidget {
    Q_OBJECT
public:
    QString  timeStr      = "00:00";
    QString  tempStr      = "-- °C";
    double   steerAngle   = 0;
    double   coolant      = 0;
    bool     canOnline    = false;
    QString  canStatus    = "SEARCHING...";
    quint32  odometer     = 0;
    double   m_fuel       = 0;

    // Warnings
    bool     warnOil      = false;
    bool     warnBat      = false;
    bool     warnABS      = false;
    bool     warnEng      = false;
    bool     warnFuel     = false;

    bool     doorDriverOpen = false;
    bool     doorPassengerOpen = false;
    bool     brakePressed   = false;
    uint8_t  accelerator    = 0;

    // Lights & Blinkers (Mapped to CanWorker Enums)
    int      blinkerStatus = 0; // 0=OFF, 1=LEFT, 2=RIGHT, 3=HAZARD
    int      lightMode     = 0; // 0=OFF, 1=LOW, 2=MEDIUM
    bool     lightHigh     = false;
    bool     lightFog      = false;

    int      wiperStatus   = 0;   // 0=off 1=low 2=high

    explicit CenterPanel(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QTimer *m_blinkTimer  = nullptr;
    bool    m_blinkState  = false;   // toggles for animation
};