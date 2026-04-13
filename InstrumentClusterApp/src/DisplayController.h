#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>

class DisplayController : public QObject {
    Q_OBJECT
public:
    explicit DisplayController(QObject *parent = nullptr);
    
    // Checks if the HDMI display accepts DDC/CI commands
    bool checkCompatibility();
    
    // Feed this the voltage (0.0v to 3.3v) from ADC
    void updateFromVoltage(double voltage);

private:
    bool m_isCompatible = false;
    int  m_currentBrightness = -1;
    bool m_isSettingBrightness = false;
    
    void setBrightness(int percentage);
};