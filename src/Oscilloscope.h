/*
* This file is part of the stmbl project.
*
* Copyright (C) 2020 Forest Darling <fdarling@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef STMBL_SERVOTERM_OSCILLOSCOPE_H
#define STMBL_SERVOTERM_OSCILLOSCOPE_H

#include "globals.h"

#include <QWidget>

namespace STMBL_Servoterm {

class Oscilloscope : public QWidget
{
    Q_OBJECT
public:
    Oscilloscope(QWidget *parent = nullptr);
public slots:
    void addChannelsSample(const QVector<float> &channelsSample);
    void resetScanning();
protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void _SetScopeX(int newX);
    QVector< QVector<float> > _channelsSamples;
    int _scopeX;
};

} // namespace STMBL_Servoterm

#endif // STMBL_SERVOTERM_OSCILLOSCOPE_H
