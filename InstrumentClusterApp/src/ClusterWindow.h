#pragma once
#include <QWidget>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusframe.h>
#include <QTimer>
#include <QVariant>
#include <QList>

#include <QtSerialBus/qcanbus.h>
#include <QtSerialBus/qcanbusdevice.h>
#include <QtSerialBus/qcanbusframe.h>

#include <QThread>
#include "CanWorker.h"

#include "DisplayController.h"

#include "GaugeWidget.h"
#include "CenterPanel.h"
#include "PillBadge.h"
#include "AppFont.h"

#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QProcess>
#include <QScreen>
#include <QGuiApplication>
#include <QtMath>
#include <QSerialPortInfo>
#include <QPaintEvent>

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>

#include <QFile>
#include <QProcess>

class GaugeWidget;
class CenterPanel;
class PillBadge;
class DebugOverlay;

class ClusterWindow : public QWidget {
    Q_OBJECT
    friend class DebugOverlay;

public:
    explicit ClusterWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void updateClock();

private:
    QThread *m_canThread;
    CanWorker *m_canWorker;

    QString m_spiStatus = "CHECKING...";
    QString m_can0Status = "CHECKING...";
    QString m_can0State = "CHECKING...";
    QStringList m_dmesgLines;

    DisplayController *m_displayCtrl;
    QString m_displayStatus = "CHECKING...";

    int m_zeroRpmCounter = 0;
    int m_zeroSpeedCounter = 0;
    
    void refreshSystemDebug();

    void updateFuelPill();
    void updateGear();
    void appendDebug(const QString &msg);

    double m_targetRpm = 0;
    double m_targetSpeed = 0;

    double m_displayRpm = 0;
    double m_displaySpeed = 0;

    enum BootState { BOOT_SWEEP_UP, BOOT_SWEEP_DOWN, BOOT_FINISHED };
    BootState m_bootState = BOOT_SWEEP_UP;

    QTimer *m_uiSmoothTimer; // A timer dedicated to the animation

    GaugeWidget  *m_rpmGauge;
    GaugeWidget  *m_spdGauge;
    PillBadge    *m_rpmPill;
    PillBadge    *m_spdPill;
    CenterPanel  *m_center;
    DebugOverlay *m_debugOverlay = nullptr;
    QTimer *m_debugRefreshTimer = nullptr;

    PillBadge *m_lcdPill;

    QTimer *m_clockTimer;

    double  m_speed    = 0;
    double  m_rpm      = 0;
    double  m_fuel     = 0;
    bool    m_canFound = false;

    bool        m_showDebug  = false;
    QStringList m_debugLines;
    int         m_canAttempts = 0;

    int m_winW = 1920;
    int m_winH = 720;
};