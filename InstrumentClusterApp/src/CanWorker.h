#pragma once
#include <QObject>
#include <QMutex>
#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusFrame>
#include <QTimer>
#include <QDebug>

enum BlinkerState { BLINKER_OFF = 0, BLINKER_LEFT = 1, BLINKER_RIGHT = 2, BLINKER_HAZARD = 3 };
enum LightState { LIGHTS_OFF = 0, LIGHTS_MINIMOS = 1, LIGHTS_MEDIOS = 2 };

struct VehicleData {
    double rpm = 0;
    double speed = 0;
    BlinkerState blinkerStatus = BLINKER_OFF;
    LightState lightMode = LIGHTS_OFF;
    bool lightHigh = false;
    bool lightFog = false;
    bool canOnline = false;

    bool doorDriverOpen = false;
    bool doorPassengerOpen = false;
    bool brakePressed = false;
    uint8_t accelerator = 0;
};

class CanWorker : public QObject {
    Q_OBJECT
public:
    explicit CanWorker(QObject *parent = nullptr);
    ~CanWorker();

    // ~Method for GUI to grab the latest data
    VehicleData getData();

public slots:
    // Started by the background thread
    void startWorker();

private slots:
    void attemptConnection();
    void readFrames();

private:
    QCanBusDevice *m_device = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    
    QMutex m_mutex;
    VehicleData m_data;

    // Debounce counters
    int m_zeroRpmCounter = 0;
    int m_zeroSpeedCounter = 0;

    // Helper macro
    unsigned char getByteRight(const QByteArray &payload, int index) {
        if (index > payload.size() || index <= 0) return 0;
        return static_cast<unsigned char>(payload[payload.size() - index]);
    }
};