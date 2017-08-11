#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QModbusDataUnit>

const int updateTime = 1000;
//Any faults in this list will be inverted.
//This value should be the bit location in the "digitalTransfer" word.
const QList<int> faultInvertList = QList<int>() << 5;

MainWindow::MainWindow(QTime firstShift, QTime secondShift, QString addr, QWidget *parent) :
    QMainWindow         (parent),
    mpUi                (new Ui::MainWindow),
    mpModbusClient      (new QModbusTcpClient(this)),
    mpTargetSeries      (new QLineSeries(this)),
    mpCycleSeries       (new QLineSeries(this)),
    mpPartsSeries       (new QLineSeries(this)),
    mpAxisY             (new QValueAxis(this)),
    mpAxisX             (new QCategoryAxis(this)),
    mRedOn              (":/images/redOn.png"),
    mRedOff             (":/images/redOff.png"),
    mRedOn_small        (":/images/redOn_small.png"),
    mRedOff_small       (":/images/redOff_small.png"),
    mGreenOn            (":/images/greenOn.png"),
    mGreenOff           (":/images/greenOff.png"),
    mBlueOn             (":/images/blueOn.png"),
    mBlueOff            (":/images/blueOff.png"),
    mVerticalFaults     (QMap<int, FaultLight*>()),
    mHorizontalFaults   (QVector<FaultLight *>()),
    mFirstShift         (firstShift),
    mSecondShift        (secondShift),
    mShiftStart         (QDateTime()),
    mShiftEnd           (QDateTime()),
    mCurrentTime        (QDateTime()),
    mShiftTarget        (10800)
{
    //Initialize UI
    mpUi->setupUi(this);
    mpUi->shiftTarget_label->setText(QString::number(mShiftTarget));
    mpUi->message_label->setText("No Alarms Detected");
    mpUi->statusBar->showMessage("Disconnected!");
    mpUi->statusBar->setStyleSheet("color: red;");

    //Initialize vertical fault lights
    FaultLight *pSpindle    = new FaultLight(mGreenOn, mGreenOff, this);
    FaultLight *pFeed       = new FaultLight(mGreenOn, mGreenOff, this);
    FaultLight *pCnc        = new FaultLight(mRedOn, mRedOff, this);
    FaultLight *pFlow       = new FaultLight(mRedOn, mRedOff, this);
    FaultLight *pChange      = new FaultLight(mBlueOn, mBlueOff, this);
    mpUi->verticalFault_layout->addWidget(pSpindle, 0, 1, 1, 1);
    mpUi->verticalFault_layout->addWidget(pFeed, 1, 1, 1, 1);
    mpUi->verticalFault_layout->addWidget(pCnc, 2, 1, 1, 1);
    mpUi->verticalFault_layout->addWidget(pFlow, 3, 1, 1, 1);
    mpUi->verticalFault_layout->addWidget(pChange, 4, 1, 1, 1);
    //The first argument passed to "insert" should be the bit location in the "digitalTransfer" word.
    //The second argument is the actual widget.
    mVerticalFaults.insert(0, pSpindle);
    mVerticalFaults.insert(1, pFeed);
    mVerticalFaults.insert(5, pCnc);
    mVerticalFaults.insert(3, pFlow);
    mVerticalFaults.insert(4, pChange);
    QMapIterator<int, FaultLight *> i(mVerticalFaults);
    while (i.hasNext())
    {
        i.next();
        FaultLight *pLight = i.value();
        pLight->setStyleSheet("border: none;");
        pLight->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    }

    //Initialize horizontal fault lights
    for (int i=1; i<9; i++)
    {
        FaultLight *pLight = new FaultLight(mRedOn_small, mRedOff_small, this);
        pLight->setStyleSheet("border: none;");
        pLight->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        mpUi->horizontalFault_layout->insertWidget(i, pLight);
        mHorizontalFaults.append(pLight);
    }
    
    //Initialize the chart
    QAreaSeries *pFillSeries = new QAreaSeries(mpPartsSeries);
    QPen pen(QBrush(), 3);
    mpTargetSeries->setPen(pen);
    mpCycleSeries->setPen(pen);
    mpPartsSeries->setPen(pen);
    pFillSeries->setPen(pen);
    mpTargetSeries->setPointsVisible(false);
    mpCycleSeries->setPointsVisible(false);
    mpPartsSeries->setPointsVisible(false);
    mpTargetSeries->setPointLabelsVisible(false);
    mpCycleSeries->setPointLabelsVisible(false);
    mpPartsSeries->setPointLabelsVisible(false);
    mpTargetSeries->setColor(QColor(255,0,0));
    mpCycleSeries->setColor(QColor(255,128,0));
    mpPartsSeries->setColor(QColor(0,0,255));
    pFillSeries->setColor(QColor(0,0,255));
    mpUi->chartView->chart()->legend()->hide();
    mpUi->chartView->chart()->addSeries(mpTargetSeries);
    mpUi->chartView->chart()->addSeries(mpCycleSeries);
    mpUi->chartView->chart()->addSeries(pFillSeries);
    mpUi->chartView->chart()->setTitle("");
    mpAxisX->setGridLineColor(QColor(Qt::gray));
    mpAxisX->setLabelsPosition(QCategoryAxis::AxisLabelsPositionOnValue);
    mpUi->chartView->chart()->addAxis(mpAxisX, Qt::AlignBottom);
    mpTargetSeries->attachAxis(mpAxisX);
    mpCycleSeries->attachAxis(mpAxisX);
    pFillSeries->attachAxis(mpAxisX);
    mpAxisY->setLabelFormat("%i");
    mpAxisY->setTitleText("");
    mpAxisY->setGridLineColor(QColor(Qt::gray));
    mpUi->chartView->chart()->addAxis(mpAxisY, Qt::AlignLeft);
    mpTargetSeries->attachAxis(mpAxisY);
    mpCycleSeries->attachAxis(mpAxisY);
    pFillSeries->attachAxis(mpAxisY);
    mpUi->chartView->setRenderHint(QPainter::Antialiasing);


    //Initialize modbus connection
    connect(mpModbusClient, &QModbusTcpClient::errorOccurred, this, &MainWindow::modbusError);
    connect(mpModbusClient, &QModbusTcpClient::stateChanged, this, &MainWindow::modbusStateChanged);
    mpModbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, addr);
    mpModbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, 503);
    mpModbusClient->setTimeout(500);
    mpModbusClient->setNumberOfRetries(1);
    if (!mpModbusClient->connectDevice()) qDebug()<<mpModbusClient->errorString();
    
    //Sets up F11 to switch between fullscreen and windowed.
    //This will not work on the PI as the program bypasses the window manager.
    //To close the program, click the Continental logo.
    QShortcut *pFullscreenShortcut = new QShortcut(QKeySequence("F11"), this);
    connect(pFullscreenShortcut, &QShortcut::activated, this, &MainWindow::toggleFullscreen);

    //Timer to update data from Modbus server as well as check if the new shift has started.
    QTimer *pUpdateTimer = new QTimer(this);
    connect(pUpdateTimer, &QTimer::timeout, this, &MainWindow::updateData);
    pUpdateTimer->start(updateTime);
    updateData();
}

MainWindow::~MainWindow()
{
    delete mpUi;
}

//Use this function when switching shifts to clear the chart.
void MainWindow::resetChart()
{
    mpTargetSeries->clear();
    mpCycleSeries->clear();
    mpPartsSeries->clear();

    //These times will not be valid until we pull the time from the modbus.
    //Until this happens we can't really touch the chart since we don't know the time.
    if (mShiftStart.isValid() && mShiftEnd.isValid())
    {
        //We need to clear all the current "Hour" labels for the X axis.
        foreach (const QString label, mpAxisX->categoriesLabels())
        {
            mpAxisX->remove(label);
        }

        //Get the total hours of the current shift.
        //Then set that many ticks (hour(s) labels)
        int hours = (mShiftStart.secsTo(mShiftEnd) / 3600);
        mpAxisX->setTickCount(hours + 1);
        mpAxisX->setRange(mShiftStart.toMSecsSinceEpoch() / 1000, mShiftEnd.toMSecsSinceEpoch() / 1000);
        for (int i=1; i<hours+1; i++)
        {
            QString label;
            if (i == 1) label = "1 Hour";
            else label = QString::number(i) + " Hours";
            mpAxisX->append(label, mShiftStart.addSecs(i * 3600).toMSecsSinceEpoch() / 1000);
        }

        mpAxisY->setRange(0, mShiftTarget);
        mpTargetSeries->append(mShiftStart.toMSecsSinceEpoch() / 1000, 0);
        mpTargetSeries->append(mShiftEnd.toMSecsSinceEpoch() / 1000, mShiftTarget);
        mpCycleSeries->append(mShiftStart.toMSecsSinceEpoch() / 1000, 0);
        mpPartsSeries->append(mShiftStart.toMSecsSinceEpoch() / 1000, 0);
    }
}

void MainWindow::updateData()
{
    //We only want to update the current time and shift info if ALL the times are valid.
    if (mCurrentTime.isValid() && mShiftStart.isValid() && mShiftEnd.isValid())
    {
        mpUi->clockLabel->setText(mCurrentTime.toString("MMMM dd, yyyy 'Time:' hh:mm:ss"));

        //Check if the next shift has started.
        if (mCurrentTime.operator >=(mShiftEnd))
        {
            //Both shift combined take 24 hours.
            //Add one day to the current shift start to get your new shift end.
            //New shift start should be current shift end.
            QDateTime tmp = mShiftStart.addDays(1);
            mShiftStart = mShiftEnd;
            mShiftEnd = tmp;

            resetChart();
        }
    }

    //If the client isn't connected let's try to connect.
    //If it is connected we need to pull the data from the Modbus server.
    if (mpModbusClient->state() == QModbusDevice::UnconnectedState)
        mpModbusClient->connectDevice();
    else if (mpModbusClient->state() == QModbusDevice::ConnectedState)
    {
        //Read only registers
        QModbusDataUnit unit1(QModbusDataUnit::InputRegisters, 0, 14);
        readData(unit1, 1);

        QModbusDataUnit unit2(QModbusDataUnit::InputRegisters, 20, 3);
        readData(unit2, 1);
    }
}

void MainWindow::modbusError(QModbusDevice::Error error)
{
    QString errorMsg("Modbus Error: " + error);
    mpUi->statusBar->showMessage(errorMsg);
    mpUi->statusBar->setStyleSheet("color: red;");
    qDebug()<<errorMsg;
}

void MainWindow::modbusStateChanged(QModbusDevice::State state)
{
    qDebug()<<"StateChanged! "<<state;
    if (state != QModbusDevice::ConnectedState)
    {
        mpUi->statusBar->showMessage("Disconnected!");
        mpUi->statusBar->setStyleSheet("color: red;");
    }
}

void MainWindow::writeData(QModbusDataUnit writeUnit, int address)
{
    if (auto *reply = mpModbusClient->sendWriteRequest(writeUnit, address))
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &MainWindow::readyRead);
        else
            reply->deleteLater();
    }
}

void MainWindow::readData(QModbusDataUnit readUnit, int address)
{
    if (auto *reply = mpModbusClient->sendReadRequest(readUnit, address))
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &MainWindow::readyRead);
        else
            reply->deleteLater();
    }
}

void MainWindow::readyRead()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
        return;

    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();
        if (unit.startAddress() == 0)
        {
            quint64 word0 = unit.value(8);
            quint64 word1 = unit.value(9);
            quint64 word2 = unit.value(10);
            quint64 word3 = unit.value(11);
            quint64 epochTime = word0 | (word1 << 16) | (word2 << 32) | (word3 << 48);
            mCurrentTime.setMSecsSinceEpoch(epochTime);


            mpUi->nameLabel->setText("WAGO_GM"+QString::number(unit.value(12)));

            //If the time from the PLC is valid and the shift times aren't we should set them.
            //This should only happen on the first modbus read. (before the PI knows the time)
            if (mCurrentTime.isValid() && (!mShiftStart.isValid() || !mShiftEnd.isValid()))
            {
                //Set both times to today just to get us in the ballpark.
                //We will fix the date later if it's needed.
                mShiftStart.setDate(mCurrentTime.date());
                mShiftEnd.setDate(mCurrentTime.date());

                //Figure out what shift we are currently in based on the current time.
                QTime now = mCurrentTime.time();
                if (now.operator >=(mFirstShift) && now.operator <(mSecondShift))
                {
                    mShiftStart.setTime(mFirstShift);
                    //We add the shift duration (absolute value of first shift to second shift) to shift start to get our shift end.
                    mShiftEnd = mShiftStart.addSecs(abs(mFirstShift.secsTo(mSecondShift)));
                }
                else
                {
                    mShiftStart.setTime(mSecondShift);
                    //Make sure we set the correct day if the shift started yesterday.
                    //This should only happen if the application starts on the next day of the second shift. (After 12am)
                    //We compare the current time (time only! eg 01:00) to first shift start time (time only! eg 07:00)
                    //If our current time is less than the first shift time, we must be in the next day of the second shift.
                    if (now.operator <(mFirstShift)) mShiftStart = mShiftStart.addDays(-1);
                    //We know that both shifts together will always take up 24 hours
                    //lets subtract first shift from 24 hours (86400 seconds) to get the length of the second shift.
                    int sec = 86400 - mFirstShift.secsTo(mSecondShift);
                    mShiftEnd = mShiftStart.addSecs(sec);
                }
                resetChart();
            }

            int cycleCount, partsCount, faults;
            cycleCount = unit.value(2);
            partsCount = unit.value(3);
            faults = unit.value(6);
            mpCycleSeries->append(mCurrentTime.toMSecsSinceEpoch() / 1000, cycleCount);
            mpPartsSeries->append(mCurrentTime.toMSecsSinceEpoch() / 1000, partsCount);

            mpUi->cycleCount_label->setText(QString::number(cycleCount));
            mpUi->partsCount_label->setText(QString::number(partsCount));

            QString curDateTimeString = mCurrentTime.toString("MMM dd, yyyy hh:mm");
            mpUi->statusBar->showMessage("Last update: " + curDateTimeString);
            mpUi->statusBar->setStyleSheet("color: black;");

            QMapIterator<int, FaultLight *> iterator(mVerticalFaults);
            while (iterator.hasNext())
            {
                iterator.next();
                //We use a QMap to store the FaultLight along with it's location in the "DigitalTransfer" word.
                //"value" holds the widget (FaultLight object) and "key" holds the bit that refers to that widget.
                //We then use the key to create a bit mask (1 << key).
                bool state = faults & (1 << iterator.key());

                //Check if the fault should be inverted.
                if (faultInvertList.contains(iterator.key()))
                    state = !state;

                iterator.value()->setState(state);
            }

            int i=0;
            int faultMatrix = (faults >> 8);
            //The horizontal fault lights at the bottom are in order of their bit location.
            //We can just use their index in the list to pull their status from the byte;
            foreach (FaultLight *light, mHorizontalFaults)
            {
                light->setState(faultMatrix & (1 << i));
                i++;
            }

            mpUi->message_label->setText(getCurrentMessage(faultMatrix));
        }
        else if (unit.startAddress() == 20)
        {
            int activeTarget = unit.value(0);
            int machineId = unit.value(1);
            mpUi->activeTarget_label->setText(QString::number(activeTarget));
            mpUi->nameLabel->setText("WAGO_GM" + QString("%1").arg(machineId, 2, 10, QChar('0')));
        }
        else
        {
            int tmp = unit.value(0);
            if (tmp != 0 && tmp != mShiftTarget)
            {
                mShiftTarget = tmp;
                mpTargetSeries->clear();
                mpTargetSeries->append(mShiftStart.toMSecsSinceEpoch() / 1000, 0);
                mpTargetSeries->append(mShiftEnd.toMSecsSinceEpoch() / 1000, mShiftTarget);
                mpAxisY->setRange(0, mShiftTarget);
            }

        }
    }
    else
    {
        QString errorMsg("Modbus read error: " + reply->errorString());
        mpUi->statusBar->showMessage(errorMsg);
        mpUi->statusBar->setStyleSheet("color: red;");
        qDebug()<<errorMsg;
    }

    reply->deleteLater();
}

QString MainWindow::getCurrentMessage(int faultMatrix)
{
    QString faultState;

    switch (faultMatrix)
    {
    //Single Input Alarms
    case 1:     faultState = "Power Present";                                                   break;
    case 2:     faultState = "End of Bar";                                                      break;
    case 4:     faultState = "Spindle Speed Fault";                                             break;
    case 64:    faultState = "Low Air Pressure";                                                break;

    //Double Input Alarms
    case 3:     faultState = "Battery Low PLC";                                                 break;
    case 5:     faultState = "Collet Area Door Open";                                           break;
    case 65:    faultState = "Low Hydraullic Oil Pressure";                                     break;
    case 6:     faultState = "Production Obtained";                                             break;
    case 20:    faultState = "Thermal Breaker";                                                 break;
    case 68:    faultState = "Mobile Work Piece Not Foward";                                    break;
    case 24:    faultState = "Low Circulating Lube Oil Level";                                  break;
    case 40:    faultState = "Overload Chip Conveyor";                                          break;

    //Triple Input Alarms
    case 19:    faultState = "Machine Encoder Failed";                                          break;
    case 35:    faultState = "Lubricating Fault";                                               break;
    case 69:    faultState = "Mobile Work Piece Underloader Not Back";                          break;
    case 25:    faultState = "Low Oil Level Total Loss System";                                 break;
    case 73:    faultState = "Feed Conveyor Not Ready";                                         break;
    case 70:    faultState = "Splash Guard Open or Gate Machine Out of Position Collets Area";  break;
    case 26:    faultState = "Threading Spindel Not Back";                                      break;
    case 74:    faultState = "Spindle Converters Not Ready";                                    break;
    case 50:    faultState = "Low Air Pressure";                                                break;
    case 82:    faultState = "Feeder Module Not Ready";                                         break;
    case 28:    faultState = "Low Flow Circulating Lube Oil";                                   break;
    case 44:    faultState = "Gate Iemca out of Position";                                      break;
    case 84:    faultState = "Overload Camshafts";                                              break;
    case 88:    faultState = "OP DW ????? (3,4,6)";                                             break;

    //Quadruple Input Alarms
    case 43:    faultState = "Stock Carrier Gate Open";                                         break;
    case 75:    faultState = "Bar Stop Out of Position";                                        break;
    case 83:    faultState = "Loading Bar";                                                     break;
    case 45:    faultState = "Converter Over Temperature";                                      break;
    case 89:    faultState = "Emergency Stop Bar Loader";                                       break;
    case 177:   faultState = "OP DW ????? (0,4,5,7)";                                           break;
    case 30:    faultState = "Threading Fault";                                                 break;
    case 172:   faultState = "Loading Bar";                                                     break;
    default:    faultState = "No Alarms Detected";
    }

    return faultState;
}

void MainWindow::toggleFullscreen()
{
    if (this->isFullScreen())
        this->showNormal();
    else
        this->showFullScreen();
}
