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

#include "ScopeDataDemux.h"
#include "globals.h"

#include <QByteArray>

namespace STMBL_Servoterm {

ScopeDataDemux::ScopeDataDemux(QObject *parent) : QObject(parent), _state(SCOPEDATADEMUX_STATE_IDLE)
{
}

QString ScopeDataDemux::addData(const QByteArray &data)
{
    QString txt;
    for (QByteArray::const_iterator it = data.begin(); it != data.end(); ++it)
    {
        if(_state == SCOPEDATADEMUX_STATE_READING_PACKET)
        {
            _packet.append((static_cast<int>(static_cast<quint8>(*it)) - 128) / 128.0);
            if(_packet.size() == SCOPE_CHANNEL_COUNT)
            {
                // save the packet
                QVector<float> packet = _packet;
                // reset the state
                _state = SCOPEDATADEMUX_STATE_IDLE;
                _packet.resize(0);
                // dispatch the packet
                emit scopePacketReceived(packet);
            }
        }
        else if (*it == static_cast<char>(0xFF))
        {
            _state = SCOPEDATADEMUX_STATE_READING_PACKET;
            _packet.resize(0);
        }
        else if (*it == static_cast<char>(0xFE))
        {
            emit scopeResetReceived();
        }
        else
        {
            txt.append(QChar::fromLatin1(*it));
        }
    }
    return txt;
}

} // namespace STMBL_Servoterm
   