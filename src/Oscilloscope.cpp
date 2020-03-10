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
    const int h = height();
    // HACK for some reason, updating less than a 4 pixel wide strip results in flickering, I need to investigate...
    update(_scopeX-1, 0, 4, h); // update affected lines
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

static void DrawSampleRange(const QVector< QVector<float> > &channelsSamples, int start, int end, int h, QPainter &painter)
{
    const int numSamples = end - start;
    if (numSamples <= 0)
        return;
    QPolygon points;
    points.reserve(numSamples);
    for (int channel = 0; channel < SCOPE_CHANNEL_COUNT; channel++)
    {
        points.resize(0);
        for (int sample = start; sample < end; sample++)
        {
            const int x = sample;
            const int y = qBound(0, static_cast<int>(h/2 - static_cast<float>(h/2)*channelsSamples[sample].at(channel)), h-1);
            points.append(QPoint(x, y));
        }
        painter.setPen(SCOPE_CHANNEL_COLORS[channel]);
        if (points.size() == 1)
            painter.drawPoint(points.at(0));
        else
            painter.drawPolyline(points);
    }
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
    if (!_channelsSamples.isEmpty())
    {
        DrawSampleRange(_channelsSamples, 0, _scopeX, h, painter);
        DrawSampleRange(_channelsSamples, _scopeX, _channelsSamples.size(), h, painter);
    }
    painter.setPen(Qt::blue);
    // painter.drawLine(_scopeX, 0, _scopeX, h-1);
    painter.drawRect(_scopeX, 0, 20, h);
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
    update(oldX, 0, 1, h);
    update(newX, 0, 1, h);
}

} // namespace STMBL_Servoterm
