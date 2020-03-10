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

#include "OscilloscopeChart.h"

#include <QChart>
#include <QLineSeries>
#include <QValueAxis>

QT_CHARTS_USE_NAMESPACE

namespace STMBL_Servoterm {

static const int SAMPLE_WINDOW_LENGTH = 200;

OscilloscopeChart::OscilloscopeChart(QWidget *parent) :
    QChartView(parent),
    _chart(new QChart),
    _chartData{},
    _chartRollingLine(new QLineSeries),
    _scopeX(0)
{
    // memset(_chartData, 0, sizeof(_chartData));
    _chart->setMinimumSize(600, 256);
    {
        QValueAxis * const axisX = new QValueAxis;
        axisX->setRange(0, SAMPLE_WINDOW_LENGTH);
        axisX->setLabelFormat("%g");
        // axisX->setTitleText("Sample");
        axisX->setVisible(false);

        QValueAxis * const axisY = new QValueAxis;
        axisY->setRange(-1, 1);
        // axisY->setTitleText("Value");
        axisY->setVisible(false);

        _chart->addAxis(axisX, Qt::AlignBottom);
        _chart->addAxis(axisY, Qt::AlignLeft);
        for (int i = 0; i < SCOPE_CHANNEL_COUNT; i++)
        {
            QLineSeries * const series = new QLineSeries;
            _chartData[i] = series;
            _chart->addSeries(series);
            series->attachAxis(axisX);
            series->attachAxis(axisY);
        }

        // set up the visual indicator of where the new data is
        // overwriting the old in the rolling oscilloscope view
        _chart->addSeries(_chartRollingLine);
        _chartRollingLine->attachAxis(axisX);
        _chartRollingLine->attachAxis(axisY);
    }
    _chart->legend()->hide();
    _chart->setTitle("Oscilloscope");
    setChart(_chart);
}

void OscilloscopeChart::addChannelsSample(const QVector<float> &channelsSample)
{
    // plot the samples
    for (int channel = 0; channel < channelsSample.size() && channel < SCOPE_CHANNEL_COUNT; channel++)
    {
        QLineSeries * const series = _chartData[channel];
        const float yValue = channelsSample[channel];
        if (_scopeX >= series->count())
            series->append(_scopeX, yValue);
        else
            series->replace(_scopeX, _scopeX, yValue);
    }
    // advance the sample index and roll around if necessary
    _scopeX++;
    if (_scopeX >= SAMPLE_WINDOW_LENGTH)
        _scopeX = 0;
    // update the incoming data vertical line indicator
    {
        QVector<QPointF> verticalLine;
        verticalLine.append(QPointF(_scopeX, -1.0));
        verticalLine.append(QPointF(_scopeX,  1.0));
        _chartRollingLine->replace(verticalLine);
    }
}

void OscilloscopeChart::resetScanning()
{
    _scopeX = 0;
}

} // namespace STMBL_Servoterm
