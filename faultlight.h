#ifndef FAULTLIGHT_H
#define FAULTLIGHT_H

#include <QObject>
#include <QLabel>

class FaultLight : public QLabel
{
    Q_OBJECT

public:
    FaultLight(const QPixmap &on, const QPixmap &off, QWidget *parent = 0);

public slots:
    void    toggleState();
    void    setState(bool);

private:
    bool    mState;
    QPixmap mOnPixmap;
    QPixmap mOffPixmap;

};

#endif // FAULTLIGHT_H
