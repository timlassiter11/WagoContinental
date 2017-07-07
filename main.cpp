#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("WagoContinental");
    a.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("ContinentalDashboard");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("server", "Modbus server IP address");

    QCommandLineOption fullscreenOption(QStringList() << "f" << "fullscreen", "Start in fullscreen mode");
    parser.addOption(fullscreenOption);

    QCommandLineOption firstShiftOption(QStringList() << "a" << "first-shift", "First shift start time (eg 0700 not 700 or 07:00)", "time");
    parser.addOption(firstShiftOption);

    QCommandLineOption secondShiftOption(QStringList() << "b" << "second-shift", "Second shift start time (eg 1900 not 19:00)", "time");
    parser.addOption(secondShiftOption);
    parser.process(a);
    const QStringList args = parser.positionalArguments();

    if (args.size() != 1)
        parser.showHelp(1);

    QTime tmp;
    QTime firstShiftTime(7, 0, 0, 0);
    QTime secondShiftTime(19, 0, 0, 0);


    //Try to get our shift times from the parser.
    //If they are not set they will return an empty QString which will cause "tmp.isValid()" to return false.
    tmp = QTime::fromString(parser.value(firstShiftOption), "hhmm");
    if (tmp.isValid())
        firstShiftTime = tmp;

    tmp = QTime::fromString(parser.value(secondShiftOption), "hhmm");
    if (tmp.isValid())
        secondShiftTime = tmp;

    MainWindow w(firstShiftTime, secondShiftTime, args.at(0));
    if (parser.isSet(fullscreenOption))
        w.showFullScreen();
    else
        w.show();

    return a.exec();
}
