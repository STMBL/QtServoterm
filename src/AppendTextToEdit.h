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

#ifndef QTSERVOTERM_APPENDTEXTTOEDIT_H
#define QTSERVOTERM_APPENDTEXTTOEDIT_H

#include <QStringList>
#include <QTextCursor>

namespace STMBL_Servoterm {

template<typename T, typename M>
static void AppendTextToEdit(T &target, M insertMethod, const QString &txt)
{
    if (!txt.isEmpty()) // may be redundant, we check outside this function
    {
        // some serious ugliness to work around a bug
        // where QTextEdit (or QTextDocument?) effectively
        // deletes trailing HTML <br/>'s, so we must use
        // plain text newlines to trick it
        const QStringList lines = txt.split("\n", QString::KeepEmptyParts);
        for (QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it)
        {
            target.moveCursor(QTextCursor::End);
            if (it != lines.begin())
            {
                target.insertPlainText("\n");
                target.moveCursor(QTextCursor::End);
            }
            if (!it->isEmpty())
            {
                (target.*insertMethod)(*it);
                target.moveCursor(QTextCursor::End);
            }
        }
    }
}

} // namespace STMBL_Servoterm

#endif // QTSERVOTERM_APPENDTEXTTOEDIT_H
