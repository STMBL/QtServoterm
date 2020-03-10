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

#ifndef STMBL_SERVOTERM_OSCILLOSCOPECHART_H
#define STMBL_SERVOTERM_OSCILLOSCOPECHART_H

#include "globals.h"

#include <QChartView>

QT_CHARTS_BEGIN_NAMESPACE
class QChartView;
class QChart;
class QLineSeries;
QT_CHARTS_END_NAMESPACE

namespace STMBL_Servoterm {

class OscilloscopeChart : public QT_CHARTS_NAMESPACE::QChartView
{
    Q_OBJECT
public:
    OscilloscopeChart(QWidget *parent = nullptr);
public slots:
    void addChannelsSample(const QVector<float> &channelsSample);
    void resetScanning();
protected:
    QT_CHARTS_NAMESPACE::QChart *_chart;
    QT_CHARTS_NAMESPACE::QLineSeries *_chartData[SCOPE_CHANNEL_COUNT];
    QT_CHARTS_NAMESPACE::QLineSeries *_chartRollingLine;
    int _scopeX;
};

} // namespace STMBL_Servoterm

#endif // STMBL_SERVOTERM_OSCILLOSCOPECHART_H
