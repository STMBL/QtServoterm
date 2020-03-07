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

#ifndef STMBL_SERVOTERM_SCOPEDATADEMUX_H
#define STMBL_SERVOTERM_SCOPEDATADEMUX_H

#include <QObject>
#include <QVector>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace STMBL_Servoterm {

class ScopeDataDemux : public QObject
{
    Q_OBJECT
    static const int SCOPE_CHANNEL_COUNT = 8;
public:
    ScopeDataDemux(QObject *parent = nullptr);
    QString addData(const QByteArray &data);
signals:
    void scopePacketReceived(const QVector<float> &packet);
    void scopeResetReceived();
protected:
    enum State
    {
        SCOPEDATADEMUX_STATE_IDLE = 0,
        SCOPEDATADEMUX_STATE_READING_PACKET
    } _state;
    QVector<float> _packet;
};

} // namespace STMBL_Servoterm

#endif // STMBL_SERVOTERM_SCOPEDATADEMUX_H
