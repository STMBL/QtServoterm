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

#include "HistoryLineEdit.h"

#include <QKeyEvent>

namespace STMBL_Servoterm {

static const int MAX_HISTORY = 100; // arbitrary, could be configurable later

HistoryLineEdit::HistoryLineEdit(QWidget *parent) : QLineEdit(parent)
{
    _history.append(QString());
    _current = _history.begin();
}

void HistoryLineEdit::saveLine()
{
    if (text().trimmed().isEmpty())
        return;
    // make sure we don't go over MAX_HISTORY entries
    // (+1 for the "stash" for the newest line)
    if (_history.size() >= MAX_HISTORY+1)
        _history.pop_back();
    _history.insert(1, text());
    _current = _history.begin();
}

void HistoryLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Up)
    {
        event->accept();
        if (!_history.isEmpty() && _current != (_history.end()-1))
        {
            if (_current == _history.begin())
            {
                // whenever we leave the first "history" item
                // we stash the "new" text being edited in it
                *_current = text();
            }
            ++_current;
            setText(*_current);
        }
    }
    else if (event->key() == Qt::Key_Down)
    {
        event->accept();
        if (!_history.isEmpty() && _current != _history.begin())
        {
            --_current;
            setText(*_current);
        }
    }
    else
        QLineEdit::keyPressEvent(event);
}

void HistoryLineEdit::keyReleaseEvent(QKeyEvent *event)
{
    QLineEdit::keyReleaseEvent(event);
}

} // namespace STMBL_Servoterm
