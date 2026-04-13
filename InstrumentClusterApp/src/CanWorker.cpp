#include "CanWorker.h"

CanWorker::CanWorker(QObject *parent) : QObject(parent) {}

CanWorker::~CanWorker() {
    if (m_device) {
        m_device->disconnectDevice();
        delete m_device;
    }
}

void CanWorker::startWorker() {
    // This runs on the new thread
    m_reconnectTimer = new QTimer(this);
    connect(m_reconnectTimer, &QTimer::timeout, this, &CanWorker::attemptConnection);
    m_reconnectTimer->start(2000);
    
    // Attempt initial connection immediately
    attemptConnection();
}

void CanWorker::attemptConnection() {
    if (m_device && m_device->state() == QCanBusDevice::ConnectedState) return;

    QString errorString;
    m_device = QCanBus::instance()->createDevice(QStringLiteral("socketcan"), QStringLiteral("can0"), &errorString);

    if (m_device && m_device->connectDevice()) {
        m_reconnectTimer->stop(); 
        connect(m_device, &QCanBusDevice::framesReceived, this, &CanWorker::readFrames);
        
        m_mutex.lock();
        m_data.canOnline = true;
        m_mutex.unlock();
    } else {
        if (m_device) {
            delete m_device;
            m_device = nullptr;
        }
    }
}

void CanWorker::readFrames() {
    if (!m_device) return;

    // Lock the mutex ONCE while processing all available frames in this run
    m_mutex.lock();

    while (m_device->framesAvailable()) {
        QCanBusFrame frame = m_device->readFrame();
        if (frame.frameType() != QCanBusFrame::DataFrame) continue;

        uint32_t id = frame.frameId();
        QByteArray payload = frame.payload();
        int dlc = payload.size();

        switch(id) {
            case 0x1F9:
            case 0x284: {
                m_data.accelerator = getByteRight(payload, 2);

                if (dlc >= 4) {
                    uint16_t rawRpm = (static_cast<unsigned char>(payload[2]) << 8) | static_cast<unsigned char>(payload[3]);
                    double tempRpm = static_cast<double>(rawRpm) / 8.0;

                    if (tempRpm > 0) {
                        m_data.rpm = qBound(0.0, tempRpm, 7000.0);
                        m_zeroRpmCounter = 0;
                    } else {
                        m_zeroRpmCounter++;
                        if (m_zeroRpmCounter > 10) {
                            m_data.rpm = 0;
                            m_zeroRpmCounter = 11;
                        }
                    }
                }
                break;
            }
            case 0x354: {
                if (dlc >= 2) {
                    uint16_t rawSpeed = ((static_cast<unsigned char>(payload[0]) << 8) | static_cast<unsigned char>(payload[1]));
                    double tempSpeed = rawSpeed / 90.0;
                    
                    if (tempSpeed > 0) {
                        m_data.speed = qBound(0.0, tempSpeed, 200.0);
                        m_zeroSpeedCounter = 0;
                    } else {
                        m_zeroSpeedCounter++;
                        if (m_zeroSpeedCounter > 10) {
                            m_data.speed = 0;
                            m_zeroSpeedCounter = 11;
                        }
                    }
                }
                break;
            }
            case 0x35D: { // Pedals
                unsigned char val = getByteRight(payload, 4);
                m_data.brakePressed = (val & 0x10);
                break;
            }
            case 0x60D: {
                // DOORS
                if (dlc >= 8) {
                    unsigned char doorByte = payload[0];
                    m_data.doorDriverOpen = (doorByte & 0x08);
                    m_data.doorPassengerOpen = (doorByte & 0x10);
                }

                // BLINKERS & FOG
                unsigned char blinkerByte = getByteRight(payload, 7);
                if      (blinkerByte == 0x36) m_data.blinkerStatus = BLINKER_LEFT;
                else if (blinkerByte == 0x56) m_data.blinkerStatus = BLINKER_RIGHT;
                else if (blinkerByte == 0x76) m_data.blinkerStatus = BLINKER_HAZARD;
                else                          m_data.blinkerStatus = BLINKER_OFF;
                
                unsigned char fogByte = getByteRight(payload, 6);
                m_data.lightFog = ((fogByte & 0x1C) == 0x1C);
                break;
            }
            case 0x625: {
                if (dlc >= 2) {
                    unsigned char byte1 = static_cast<unsigned char>(payload[1]);
                    
                    // Bitmasking
                    // 0x60 has the bits for Medium. 0x40 has the bits for Low.
                    if ((byte1 & 0x60) == 0x60) {
                        m_data.lightMode = LIGHTS_MEDIOS;
                    } else if ((byte1 & 0x40) == 0x40) {
                        m_data.lightMode = LIGHTS_MINIMOS;
                    } else {
                        m_data.lightMode = LIGHTS_OFF;
                    }

                    // Still to check High Beams
                    m_data.lightHigh = (byte1 & 0x04) || (byte1 & 0x08) || (byte1 & 0x80);
                }
                break;
            }
        }
    }
    m_mutex.unlock();
}

VehicleData CanWorker::getData() {
    QMutexLocker locker(&m_mutex); // Automatically locks, and unlocks when function returns
    return m_data;
}