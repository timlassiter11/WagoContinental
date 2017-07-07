#include "faultlight.h"

FaultLight::FaultLight(const QPixmap &on, const QPixmap &off, QWidget *parent) :
    QLabel(parent),
    mState(false),
    mOnPixmap(on),
    mOffPixmap(off)
{
    this->setPixmap(mOffPixmap);
}

void FaultLight::toggleState()
{
    mState = !mState;
    if (mState)
        this->setPixmap(mOnPixmap);
    else
        this->setPixmap(mOffPixmap);
}

void FaultLight::setState(bool set)
{
    if (set != mState)
    {
        mState = set;
        if (mState)
            this->setPixmap(mOnPixmap);
        else
            this->setPixmap(mOffPixmap);
    }
}
