#pragma once
#include <QWidget>
#include <QString>

class PillBadge : public QWidget {
    Q_OBJECT
public:
    QString leftLabel  = "FUEL";
    QString midValue   = "47%";
    QString rightValue = "250KM";

    explicit PillBadge(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;
};