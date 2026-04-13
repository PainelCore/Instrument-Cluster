#include "DisplayController.h"
#include <QDebug>
#include <QtMath>

DisplayController::DisplayController(QObject *parent) : QObject(parent) {
}

bool DisplayController::checkCompatibility() {
    // Run "ddcutil detect" to see if the HDMI I2C bus responds
    QProcess ddcProcess;
    ddcProcess.start("ddcutil", {"detect"});
    ddcProcess.waitForFinished(3000); // Wait up to 3 seconds for initial detection
    
    QString output = QString::fromUtf8(ddcProcess.readAllStandardOutput());
    
    // If it found a valid VCP (Virtual Control Panel) display, it's compatible
    if (output.contains("Display 1") && !output.contains("Invalid display")) {
        m_isCompatible = true;
        qDebug() << "[DISPLAY] ddcutil compatible display found!";
        return true;
    }
    
    qDebug() << "[DISPLAY] Display not compatible or I2C not enabled.";
    m_isCompatible = false;
    return false;
}

void DisplayController::updateFromVoltage(double voltage) {
    if (!m_isCompatible) return;

    // Constrain voltage between 0.0 and 3.3 just in case
    double safeVolt = qBound(0.0, voltage, 3.3);

    // Math: Map 0.0V -> 100%, and 3.3V -> 20%
    // 100 - ( (voltage / 3.3) * 80 )
    int targetPct = qRound(100.0 - ((safeVolt / 3.3) * 80.0));

    // Only update if the brightness changed by at least 2% to prevent micro-flickering
    if (qAbs(targetPct - m_currentBrightness) >= 2) {
        setBrightness(targetPct);
    }
}

void DisplayController::setBrightness(int percentage) {
    // Prevent stacking commands if ddcutil is already running
    if (m_isSettingBrightness) return;
    
    m_isSettingBrightness = true;
    m_currentBrightness = percentage;
    
    // We run this asynchronously so it doesn't freeze the gauge sweep!
    QProcess *process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process]() {
        m_isSettingBrightness = false;
        process->deleteLater();
    });
    
    // VCP code 10 is standard for Brightness
    process->start("ddcutil", {"setvcp", "10", QString::number(percentage)});
}