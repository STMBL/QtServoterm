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

#include "Actions.h"

namespace STMBL_Servoterm {

Actions::Actions(QObject *parent) : QObject(parent)
{
    fileQuit = new QAction("&Quit", this);
    connectionConnect = new QAction("Connect", this);
    connectionDisconnect = new QAction("Disconnect", this);
    driveEnable = new QAction("Enable", this);
    driveDisable = new QAction("Disable", this);
    driveJogEnable = new QAction("Jog", this);
    driveEditConfig = new QAction("Config", this);
    dataRecord = new QAction("Record", this);
    dataSetDirectory = new QAction("Set Directory...", this);
    dataOpenDirectory = new QAction("Open Directory (in File Manager)", this);
    viewOscilloscope = new QAction("Show Oscilloscope", this);
    viewXYScope = new QAction("Show X/Y Scope", this);
    viewConsole = new QAction("Show Console Output", this); // TODO change this to "Show Console"
    viewClearConsole = new QAction("Clear", this); // TODO change this to "Clear Console"?
    driveJogEnable->setCheckable(true);
    dataRecord->setCheckable(true);
    viewOscilloscope->setCheckable(true);
    viewXYScope->setCheckable(true);
    viewConsole->setCheckable(true);
}

} // namespace STMBL_Servoterm
