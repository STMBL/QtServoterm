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

#include "Oscilloscope.h"

#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>

namespace STMBL_Servoterm {

static const QColor SCOPE_CHANNEL_COLORS[SCOPE_CHANNEL_COUNT] =
{
    Qt::black,
    Qt::red,
    Qt::blue,
    Qt::green,
    QColor(255, 128, 0),
    QColor(128, 128, 64),
    QColor(128, 64, 128),
    QColor(64, 128, 128)
};

Oscilloscope::Oscilloscope(QWidget *parent) : QWidget(parent), _scopeX(0)
{
    setMinimumSize(600, 256);
    QPalette pal = palette();
    pal.setColor(QPalette::Background, Qt::white);
    setAutoFillBackground(true);
    setPalette(pal);
}

void Oscilloscope::addChannelsSample(const QVector<float> &channelsSample)
{
    if (channelsSample.size() != SCOPE_CHANNEL_COUNT) // sanity check
        return;

    // add/overwrite the appropriate sample
    if (_scopeX < _channelsSamples.size())
        _channelsSamples[_scopeX] = channelsSample;
    else
        _channelsSamples.append(channelsSample);

    // update the next position to write to
    // NOTE: has the side effect of updating the region
    //       where we just layed down a sample
    if (_scopeX+1 >= width())
        _SetScopeX(0);
    else
        _SetScopeX(_scopeX + 1);
}

void Oscilloscope::resetScanning()
{
    _SetScopeX(0);
}

void Oscilloscope::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    // painter.setRenderHint(QPainter::Antialiasing);
    const int h = height();
    const int w = width();
    painter.setPen(Qt::gray);
    painter.drawLine(0, h/2, w-1, h/2);

    // a very slight optimization
    if (_channelsSamples.isEmpty())
        return;

    QVector<QPoint> points;
    const int numSamples = _channelsSamples.size();
    points.resize(numSamples);
    for (int channel = 0; channel < SCOPE_CHANNEL_COUNT; channel++)
    {
        for (int sample = 0; sample < numSamples; sample++)
        {
            const int x = sample;
            const int y = qBound(0, static_cast<int>(h/2 - static_cast<float>(h/2)*_channelsSamples[sample].at(channel)), h-1);
            points[sample] = QPoint(x, y);
        }
        painter.setPen(SCOPE_CHANNEL_COLORS[channel]);
        painter.drawPolyline(points);
    }
}

void Oscilloscope::resizeEvent(QResizeEvent *event)
{
    const int w = event->size().width();

    // possibly reduce the data window length
    if (_channelsSamples.size() > w)
        _channelsSamples.resize(w);
    // make sure we're still inside the window
    if (_scopeX >= _channelsSamples.size())
        _SetScopeX(0);
}

void Oscilloscope::_SetScopeX(int newX)
{
    const int h = height();
    const int oldX = _scopeX;
    _scopeX = newX;
    update(QRect(oldX, 0, 1, h));
    update(QRect(newX, 0, 1, h));
}

} // namespace STMBL_Servoterm
