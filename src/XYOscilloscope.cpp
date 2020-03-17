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

#include "XYOscilloscope.h"

#include <QPaintEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QTimer>

namespace STMBL_Servoterm {

static const int MINIMUM_PLOT_SIZE = 256;

XYOscilloscope::XYOscilloscope(QWidget *parent) :
    QWidget(parent),
    _plot(QSize(MINIMUM_PLOT_SIZE, MINIMUM_PLOT_SIZE), QImage::Format_RGB32),
    _timer(new QTimer(this))
{
    _timer->setInterval(50);
    connect(_timer, SIGNAL(timeout()), this, SLOT(slot_FadeTimeout()));
    setMinimumSize(MINIMUM_PLOT_SIZE, MINIMUM_PLOT_SIZE); // TODO set a square aspect ratio somehow
    _plot.fill(Qt::white);
}

void XYOscilloscope::addChannelsSample(const QVector<float> &channelsSample)
{
    // update the off-screen buffer
    QPoint pt(128+channelsSample.at(0)*128, 128-channelsSample.at(1)*128);
    _plot.setPixel(pt, QColor(Qt::blue).rgba());
    _points.insert(pt);

    // calculate what it would affect in screen coordinates
    update(_ImageRectToWidgetRect(QRect(pt, QSize(1, 1))));

    // make sure fading is re-enabled
    if (!_timer->isActive())
        _timer->start();
}

void XYOscilloscope::resetScanning()
{
}

// apparently the API changed
#if QT_VERSION > QT_VERSION_CHECK(5, 10, 0)
    #define SIZE_IN_BYTES_METHOD sizeInBytes
#else
    #define SIZE_IN_BYTES_METHOD byteCount
#endif

void XYOscilloscope::slot_FadeTimeout()
{
    QRect region; // could be done with QRegion too
    const QRgb whiteVal = QColor(Qt::white).rgba();
    for (QSet<QPoint>::iterator it = _points.begin(); it != _points.end(); /* */)
    {
        QRgb val = _plot.pixel(*it);
        val += 0x00010100; // increase the red and green channels (causes the solid blue pixel to fade to white)
        _plot.setPixel(*it, val);
        region = region.united(QRect(*it, QSize(1, 1)));
        if (val == whiteVal)
        {
            it = _points.erase(it);
        }
        else // not fully faded yet
        {
            ++it;
        }
    }

    // if we are done, then disable the timer for performance
    if (_points.isEmpty())
        _timer->stop();

    // redraw
    update(_ImageRectToWidgetRect(region));
}

void XYOscilloscope::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    const int h = height();
    const int w = width();
    painter.drawImage(rect(), _plot);
    painter.setPen(Qt::gray);
    painter.drawEllipse(QRect(w/8, h/8, w*3/4-1, h*3/4-1)); // 3/4 size circle
    // painter.drawEllipse(QRect(0, 0, w-1, h-1)); // full size circle
}

void XYOscilloscope::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

QRect XYOscilloscope::_ImageRectToWidgetRect(const QRect &r) const
{
    const qreal xScalar = static_cast<qreal>( width())/MINIMUM_PLOT_SIZE;
    const qreal yScalar = static_cast<qreal>(height())/MINIMUM_PLOT_SIZE;
    const QTransform trans = QTransform::fromScale(xScalar, yScalar);
    const QRect widgetPixelRect = trans.mapRect(r);
    return widgetPixelRect;
}

} // namespace STMBL_Servoterm
