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

#ifndef STMBL_SERVOTERM_XYOSCILLOSCOPE_H
#define STMBL_SERVOTERM_XYOSCILLOSCOPE_H

#include "globals.h"

#include <QWidget>
#include <QImage>
#include <QSet>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

inline uint qHash(const QPoint &pt)
{
    return (pt.y() << 16) + pt.x();
}

namespace STMBL_Servoterm {

class XYOscilloscope : public QWidget
{
    Q_OBJECT
public:
    XYOscilloscope(QWidget *parent = nullptr);
public slots:
    void addChannelsSample(const QVector<float> &channelsSample);
    void resetScanning();
protected slots:
    void slot_FadeTimeout();
protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    QRect _ImageRectToWidgetRect(const QRect &r) const;
    QImage _plot;
    QTimer *_timer;
    QSet<QPoint> _points;
};

} // namespace STMBL_Servoterm

#endif // STMBL_SERVOTERM_XYOSCILLOSCOPE_H
