#include <QApplication>
#include <QFontDatabase>
#include <QFont>
#include "ClusterWindow.h"

int main(int argc, char *argv[]) {
#ifndef DESKTOP_MOCK
    qputenv("QT_QPA_PLATFORM", "linuxfb:fb=/dev/fb0");
    qputenv("QT_QPA_FONTDIR",  "/usr/lib/fonts");
#endif

    QApplication a(argc, argv);

    // Load embedded font from Qt resource
    int id = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans.ttf");
    int idB = QFontDatabase::addApplicationFont(":/fonts/DejaVuSans-Bold.ttf");

    QString family = "DejaVu Sans";
    if (id < 0) {
        id = QFontDatabase::addApplicationFont("/usr/lib/fonts/DejaVuSans.ttf");
        if (id >= 0) {
            QStringList fam = QFontDatabase::applicationFontFamilies(id);
            if (!fam.isEmpty()) family = fam.first();
        }
    } else {
        QStringList fam = QFontDatabase::applicationFontFamilies(id);
        if (!fam.isEmpty()) family = fam.first();
    }
    Q_UNUSED(idB)

    qputenv("CLUSTER_FONT", family.toUtf8());

    QFont f(family, 12);
    f.setStyleHint(QFont::SansSerif);
    a.setFont(f);

    ClusterWindow w;

#ifdef DESKTOP_MOCK
    w.setFixedSize(1920, 720);
    w.show();
#else
    w.showFullScreen();
#endif

    return a.exec();
}