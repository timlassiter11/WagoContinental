#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "faultlight.h"

#include <QtCharts>
#include <QMainWindow>
#include <QModbusDevice>
#include <QModbusTcpClient>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QTime firstShift, QTime secondShift, QString addr, QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow          *mpUi;
    QModbusTcpClient        *mpModbusClient;
    QLineSeries             *mpTargetSeries;
    QLineSeries             *mpCycleSeries;
    QLineSeries             *mpPartsSeries;
    QValueAxis              *mpAxisY;
    QCategoryAxis           *mpAxisX;
    const QPixmap           mRedOn;
    const QPixmap           mRedOff;
    const QPixmap           mRedOn_small;
    const QPixmap           mRedOff_small;
    const QPixmap           mGreenOn;
    const QPixmap           mGreenOff;
    const QPixmap           mBlueOn;
    const QPixmap           mBlueOff;
    QMap<int, FaultLight *> mVerticalFaults;
    QVector<FaultLight *>   mHorizontalFaults;
    QTime                   mFirstShift;
    QTime                   mSecondShift;
    QDateTime               mShiftStart;
    QDateTime               mShiftEnd;
    QDateTime               mCurrentTime;
    int                     mShiftTarget;

    void                    resetChart();

private slots:
    void                    updateData();
    void                    modbusError(QModbusDevice::Error error);
    void                    modbusStateChanged(QModbusTcpClient::State state);
    void                    readData(QModbusDataUnit readUnit, int address);
    void                    writeData(QModbusDataUnit writeUnit, int address);
    void                    readyRead();
    void                    toggleFullscreen();
};

#endif // MAINWINDOW_H
