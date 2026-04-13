#include "ClusterWindow.h"

class DebugOverlay : public QWidget {
public:
    ClusterWindow* m_parent;

    DebugOverlay(ClusterWindow* parent) : QWidget(parent), m_parent(parent) {}

protected:
    void paintEvent(QPaintEvent *) override {
        if (!m_parent->m_showDebug) return;

        QPainter p(this);
        // Semi-transparent dark panel over entire screen
        p.fillRect(rect(), QColor(0, 0, 0, 200));

        double pad  = 40;
        double lineH = 28;
        double startY = pad;

        // Title
        p.setPen(QColor(180, 95, 15));
        p.setFont(QFont(appFont(), 20, QFont::Bold));
        p.drawText(QRectF(pad, startY, m_parent->m_winW - pad*2, 36),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   "CLUSTER DEBUG — Waiting for CAN bus...");
        startY += 50;

        // Divider
        p.setPen(QPen(QColor(80, 45, 8), 1));
        p.drawLine(QPointF(pad, startY), QPointF(m_parent->m_winW - pad, startY));
        startY += 16;

        // System info block
        p.setFont(QFont(appFont(), 13));

        auto drawInfo = [&](QString label, QString value, QColor valColor) {
            p.setPen(QColor(130, 90, 40));
            p.drawText(QRectF(pad, startY, 280, lineH),
                       Qt::AlignLeft | Qt::AlignVCenter, label);
            p.setPen(valColor);
            p.drawText(QRectF(pad + 290, startY, m_parent->m_winW - pad*2 - 290, lineH),
                       Qt::AlignLeft | Qt::AlignVCenter, value);
            startY += lineH;
        };

        drawInfo("Screen size:",
                 QString("%1 x %2").arg(m_parent->m_winW).arg(m_parent->m_winH),
                 QColor(240, 235, 225));

        drawInfo("CAN attempts:",
                 QString::number(m_parent->m_canAttempts),
                 m_parent->m_canFound ? QColor(32, 200, 80) : QColor(220, 50, 50));

        drawInfo("CAN status:",
                 m_parent->m_center->canStatus,
                 m_parent->m_canFound ? QColor(32, 200, 80) : QColor(220, 50, 50));

        // Check SPI devices
        drawInfo("SPI Status:", m_parent->m_spiStatus, (m_parent->m_spiStatus == "FOUND" ? Qt::green : Qt::red));

        // Check CAN interface
        drawInfo("can0 Status:", m_parent->m_can0Status, (m_parent->m_can0Status == "EXISTS" ? Qt::green : Qt::red));
        drawInfo("can0 State:", m_parent->m_can0State, (m_parent->m_can0State == "UP" ? Qt::green : Qt::red));

        // Font status
        drawInfo("Font family:",
                 QString::fromUtf8(qgetenv("CLUSTER_FONT")),
                 QColor(240, 235, 225));

        // Qt platform
        drawInfo("Qt platform:",
                 QString::fromUtf8(qgetenv("QT_QPA_PLATFORM")),
                 QColor(240, 235, 225));

        startY += 8;
        p.setPen(QPen(QColor(80, 45, 8), 1));
        p.drawLine(QPointF(pad, startY), QPointF(m_parent->m_winW - pad, startY));
        startY += 14;

        // Log lines
        p.setFont(QFont(appFont(), 11));
        for (const QString &line : m_parent->m_debugLines) {
            if (startY + lineH > m_parent->m_winH - pad) break;
            QColor lineCol = line.contains("ERROR") || line.contains("FAIL")
                           ? QColor(220, 80, 80)
                           : line.contains("OK") || line.contains("ONLINE")
                             ? QColor(32, 200, 80)
                             : QColor(160, 130, 80);
            p.setPen(lineCol);
            p.drawText(QRectF(pad, startY, m_parent->m_winW - pad*2, lineH),
                       Qt::AlignLeft | Qt::AlignVCenter, line);
            startY += lineH;
        }

        // Show dmesg CAN/SPI lines WITHOUT blocking the UI thread
        startY += 8;
        p.setPen(QColor(180, 95, 15));
        p.setFont(QFont(appFont(), 11, QFont::Bold));
        p.drawText(QRectF(pad, startY, m_parent->m_winW - pad*2, lineH),
                Qt::AlignLeft, "dmesg (SPI/CAN):");
        startY += lineH;

        p.setFont(QFont(appFont(), 10));
        for (const QString &line : m_parent->m_dmesgLines) {
            QString low = line.toLower();
            if (startY + lineH > m_parent->m_winH - 40) break;

            bool isErr = low.contains("error") || low.contains("fail");
            p.setPen(isErr ? QColor(220, 80, 80) : QColor(160, 200, 130));
            p.drawText(QRectF(pad, startY, m_parent->m_winW - pad*2, lineH),
                    Qt::AlignLeft | Qt::AlignVCenter,
                    line.trimmed().left(120));
            startY += lineH;
        }

        // Bottom hint
        p.setPen(QColor(80, 55, 20));
        p.setFont(QFont(appFont(), 11));
        p.drawText(QRectF(pad, m_parent->m_winH - 36, m_parent->m_winW - pad*2, 28),
                   Qt::AlignCenter,
                   "Debug overlay auto-hides when CAN comes online");
    }
};

ClusterWindow::ClusterWindow(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setStyleSheet("background-color: #080502;");

#ifdef DESKTOP_MOCK
    m_winW = 1920;
    m_winH = 720;
    setFixedSize(m_winW, m_winH);
#else
    QScreen *screen = qApp->primaryScreen();
    if (screen) {
        m_winW = screen->size().width();
        m_winH = screen->size().height();
    } else {
        m_winW = 1920;
        m_winH = 720;
    }
#endif

    m_debugRefreshTimer = new QTimer(this);
    connect(m_debugRefreshTimer, &QTimer::timeout, this, &ClusterWindow::refreshSystemDebug);
    
    // Only start polling background shell commands if debug is visible
    if (m_showDebug) {
        m_debugRefreshTimer->start(2000); 
        refreshSystemDebug(); 
    }

    const int gaugeSize = m_winH - 60;
    const int gaugeTop  = 10;
    const int pillW     = qRound(gaugeSize * 0.56);
    const int pillH     = qRound(m_winH * 0.042);
    const int pillY     = gaugeTop + gaugeSize + 8;
    const int gaugeL    = qRound(m_winW * 0.010);
    const int gaugeR    = m_winW - gaugeL - gaugeSize;
    const int centerX   = gaugeL + gaugeSize + qRound(m_winW * 0.010);
    const int centerW   = gaugeR - centerX - qRound(m_winW * 0.010);

    // ── RPM gauge ─────────────────────────────────────────────────────────
    GaugeWidget::Config rpmCfg;
    rpmCfg.unit           = "RPM";
    rpmCfg.minVal         = 0;
    rpmCfg.maxVal         = 7000;
    rpmCfg.tickStep       = 1000;
    rpmCfg.arcStartPct    = 0.62;
    rpmCfg.arcSpanPct     = 0.76;
    rpmCfg.arcActiveColor = QColor(180, 95, 15);

    m_rpmGauge = new GaugeWidget(rpmCfg, this);
    m_rpmGauge->setGeometry(gaugeL, gaugeTop, gaugeSize, gaugeSize);

    m_rpmPill = new PillBadge(this);
    m_rpmPill->setGeometry(gaugeL + (gaugeSize - pillW) / 2,
                            pillY, pillW, pillH);
    m_rpmPill->leftLabel  = "FUEL";
    m_rpmPill->midValue   = "0%";
    m_rpmPill->rightValue = "---KM";

    // ── Speed gauge ───────────────────────────────────────────────────────
    GaugeWidget::Config spdCfg;
    spdCfg.unit           = "km/h";
    spdCfg.minVal         = 0;
    spdCfg.maxVal         = 200;
    spdCfg.tickStep       = 10;
    spdCfg.arcStartPct    = 0.62;
    spdCfg.arcSpanPct     = 0.76;
    spdCfg.arcActiveColor = QColor(180, 95, 15);

    m_spdGauge = new GaugeWidget(spdCfg, this);
    m_spdGauge->setGeometry(gaugeR, gaugeTop, gaugeSize, gaugeSize);

    m_spdPill = new PillBadge(this);
    m_spdPill->setGeometry(gaugeR + (gaugeSize - pillW) / 2,
                            pillY, pillW, pillH);
    m_spdPill->leftLabel  = "BATTERY";
    m_spdPill->midValue   = "--V";
    m_spdPill->rightValue = "0V";

    // ── Center panel ──────────────────────────────────────────────────────
    m_center = new CenterPanel(this);
    m_center->setGeometry(centerX, 0, centerW, m_winH);

    // ── Setup Display Controller ──────────────────────────────────────────
    m_displayCtrl = new DisplayController(this);
    if (m_displayCtrl->checkCompatibility()) {
        m_displayStatus = "DDC/CI OK";
    } else {
        m_displayStatus = "NO DDC/CI";
    }

    m_lcdPill = new PillBadge(this);

    m_lcdPill->setGeometry(centerX + (centerW - pillW) / 2, 
                           pillY, 
                           pillW, pillH);
                           
    m_lcdPill->leftLabel  = "DISPLAY";
    m_lcdPill->midValue   = m_displayStatus;
    m_lcdPill->rightValue = "";

#ifndef DESKTOP_MOCK
    // ── Setup Background Thread for CAN ───────────────────────────────────
    m_canThread = new QThread(this);
    m_canWorker = new CanWorker();
    
    // Move the worker to the background thread
    m_canWorker->moveToThread(m_canThread);
    
    // When the thread starts, tell the worker to begin connecting/listening
    connect(m_canThread, &QThread::started, m_canWorker, &CanWorker::startWorker);
    
    // Ensure memory is cleaned up when the app closes
    connect(m_canThread, &QThread::finished, m_canWorker, &QObject::deleteLater);
    
    m_canThread->start();

    // ── Setup Hardware GPIO for Blinker Buzzer (Using GPIO 26 / Pin 37) ──
    QProcess::execute("sh", {"-c", "if [ ! -d /sys/class/gpio/gpio26 ]; then echo 26 > /sys/class/gpio/export; fi"});
    QProcess::execute("sh", {"-c", "echo out > /sys/class/gpio/gpio26/direction"});
    QProcess::execute("sh", {"-c", "echo 0 > /sys/class/gpio/gpio26/value"});

    // ── UI Rendering Loop (30 FPS) ────────────────────────────────────────
    m_uiSmoothTimer = new QTimer(this);
    connect(m_uiSmoothTimer, &QTimer::timeout, this, [this]() {

        // ── Normal Operation (Boot Finished) ──
        
        VehicleData data = m_canWorker->getData();

        m_rpm = data.rpm;
        m_speed = data.speed;

        m_rpmGauge->setValue(m_rpm);
        m_spdGauge->setValue(m_speed);

        m_center->blinkerStatus = data.blinkerStatus;
        // ── Hardware Buzzer Trigger ──
        // data.blinkerStatus pulses from the CAN bus (1 -> 0 -> 1 -> 0),
        static int lastBlinkerStatus = 0;
        if (data.blinkerStatus != lastBlinkerStatus) {
            lastBlinkerStatus = data.blinkerStatus;
            
            QFile gpioFile("/sys/class/gpio/gpio26/value");
            if (gpioFile.open(QIODevice::WriteOnly)) {
                // Write '1' if blinker is active (Left, Right, or Hazard), else '0'
                gpioFile.write(data.blinkerStatus != 0 ? "1" : "0");
                gpioFile.close();
            }
        }

        m_center->lightMode = data.lightMode;
        m_center->lightHigh = data.lightHigh;
        m_center->lightFog = data.lightFog;
        
        m_center->doorDriverOpen = data.doorDriverOpen;
        m_center->doorPassengerOpen = data.doorPassengerOpen;
        m_center->brakePressed = data.brakePressed;
        m_center->accelerator = data.accelerator;
        
        if (data.canOnline && !m_center->canOnline) {
            m_center->canOnline = true;
            m_center->canStatus = "CANBUS ONLINE";
            m_canFound = true;
            
            if (m_showDebug) {
                m_showDebug = false;
                if (m_debugRefreshTimer) {
                    m_debugRefreshTimer->stop();
                    m_debugRefreshTimer->deleteLater();
                    m_debugRefreshTimer = nullptr;
                }
                if (m_debugOverlay) {
                    m_debugOverlay->hide();
                    m_debugOverlay->deleteLater(); 
                    m_debugOverlay = nullptr;
                }
            }
        }

        m_center->update();
        updateFuelPill();
        updateGear(); 
    });
    
    m_uiSmoothTimer->start(33);
#endif

#ifndef DESKTOP_MOCK
    // Zero state on Pi
    m_rpmGauge->setValue(0);
    m_spdGauge->setValue(0);
    m_center->coolant    = 0;
    m_center->m_fuel     = 0;
    m_center->steerAngle = 0;
    m_center->odometer   = 0;
    m_center->canOnline  = false;
    m_center->canStatus  = "WAITING FOR CAN...";
    m_center->tempStr    = "-- °C";
#endif

    // ── Timers ────────────────────────────────────────────────────────────
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &ClusterWindow::updateClock);
    m_clockTimer->start(1000);

    appendDebug("ClusterWindow initialized");
    appendDebug(QString("Screen: %1x%2").arg(m_winW).arg(m_winH));

#ifdef DESKTOP_MOCK
    QTimer *mockTimer = new QTimer(this);
    connect(mockTimer, &QTimer::timeout, this, [this]() {
        static double t = 0;
        t += 0.04;
        m_rpm   = 1000 + 2500 * (0.5 + 0.5 * qSin(t));
        m_speed = 60   + 60   * (0.5 + 0.5 * qSin(t * 0.7));
        m_fuel  = 0.3  + 0.3  * (0.5 + 0.5 * qSin(t * 0.13));
        m_rpmGauge->setValue(m_rpm);
        m_spdGauge->setValue(m_speed);
        m_center->steerAngle  = 80.0 * qSin(t * 0.3);
        m_center->coolant     = 80.0 + 20.0 * qSin(t * 0.1);
        m_center->odometer    = 138021;
        m_center->canOnline   = true;
        m_center->canStatus   = "MOCK MODE";
        m_center->warnOil     = qSin(t * 0.2) > 0.5;
        
        // ── FIXED VARIABLES ──
        // 2 = MEDIOS (Low Beam on). 0 = OFF.
        m_center->lightMode   = (qSin(t * 0.15) > 0) ? 2 : 0; 
        m_center->lightHigh   = qSin(t * 0.1) > 0.7;
        m_center->m_fuel      = m_fuel;
        
        // 1 = Left, 2 = Right, 0 = Off
        if (qSin(t * 0.08) > 0.5) {
            m_center->blinkerStatus = 1; 
        } else if (qSin(t * 0.06) > 0.7) {
            m_center->blinkerStatus = 2; 
        } else {
            m_center->blinkerStatus = 0; 
        }

        m_center->update();
        updateFuelPill();

        // Hide debug overlay in mock mode after 3 seconds
        static int mockTick = 0;
        if (++mockTick > 60) {
            if (m_showDebug) {
                m_showDebug = false;
                
                if (m_debugRefreshTimer) {
                    m_debugRefreshTimer->stop();
                    m_debugRefreshTimer->deleteLater();
                    m_debugRefreshTimer = nullptr;
                }
                
                if (m_debugOverlay) {
                    m_debugOverlay->hide();
                    m_debugOverlay->deleteLater(); 
                    m_debugOverlay = nullptr;
                }
            }
        }
    });
    mockTimer->start(50);
#endif

    // Initialize overlay and raise it above gauges
    m_debugOverlay = new DebugOverlay(this);
    m_debugOverlay->resize(m_winW, m_winH);
    m_debugOverlay->raise(); 
    
    if (!m_showDebug) {
        m_debugOverlay->hide();
    }
}

void ClusterWindow::appendDebug(const QString &msg) {
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_debugLines.append(QString("[%1] %2").arg(ts).arg(msg));
    
    // Keep last 20 lines
    while (m_debugLines.size() > 20)
        m_debugLines.removeFirst();
        
    update();
    if (m_debugOverlay) m_debugOverlay->update();
}

void ClusterWindow::paintEvent(QPaintEvent *pe) {
    QPainter p(this);
    
    if (pe->rect() == rect()) {
        p.fillRect(rect(), QColor(8, 5, 2));
    }
}

void ClusterWindow::updateClock() {
    m_center->timeStr = QDateTime::currentDateTime().toString("HH:mm");
    m_center->update();
}

void ClusterWindow::updateFuelPill() {
    int pct = qRound(m_fuel * 100);
    m_rpmPill->midValue = QString("%1%").arg(pct);
    m_rpmPill->update();
}

void ClusterWindow::updateGear() {
    int gear = 0;
    if (m_speed < 3) {
        gear = 0;
    } else if (m_rpm > 100) {
        double ratio = m_speed / (m_rpm / 1000.0);
        if      (ratio < 6)  gear = 1;
        else if (ratio < 11) gear = 2;
        else if (ratio < 16) gear = 3;
        else if (ratio < 22) gear = 4;
        else if (ratio < 30) gear = 5;
        else                 gear = 6;
    }
    Q_UNUSED(gear)
}

void ClusterWindow::refreshSystemDebug() {
    if (!m_showDebug) return;

    // 1. Check SPI
    QProcess lsSpi;
    lsSpi.start("ls", {"/dev/spidev0.0"});
    lsSpi.waitForFinished(100); // Short timeout
    m_spiStatus = (lsSpi.exitCode() == 0) ? "FOUND" : "NOT FOUND";

    // 2. Check CAN Interface
    QProcess ipLink;
    ipLink.start("ip", {"-brief", "link", "show", "can0"});
    ipLink.waitForFinished(100);
    QString ipOut = QString::fromUtf8(ipLink.readAllStandardOutput()).trimmed();
    m_can0Status = ipOut.isEmpty() ? "NOT FOUND" : "EXISTS";
    m_can0State = ipOut.contains("UP") ? "UP" : "DOWN";

    // 3. Get dmesg (last 5 relevant lines)
    QProcess dmesg;
    dmesg.start("sh", {"-c", "dmesg | grep -iE 'spi|mcp|can' | tail -n 5"});
    dmesg.waitForFinished(200);
    m_dmesgLines = QString::fromUtf8(dmesg.readAllStandardOutput()).split('\n');
}