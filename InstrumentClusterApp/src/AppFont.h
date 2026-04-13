#pragma once
#include <QString>
#include <QByteArray>

inline QString appFont() {
    QByteArray f = qgetenv("CLUSTER_FONT");
    return f.isEmpty() ? QString("sans") : QString::fromUtf8(f);
}