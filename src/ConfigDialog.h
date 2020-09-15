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

#ifndef QTSERVOTERM_CONFIGDIALOG_H
#define QTSERVOTERM_CONFIGDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace STMBL_Servoterm {

class SerialConnection;

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    ConfigDialog(SerialConnection *serialConnection, QWidget *parent = nullptr);
    ~ConfigDialog();
public slots:
    void appendConfigLine(const QString &configLine);
protected slots:
    void slot_SaveClicked();
    void slot_ConfigTextChanged();
protected:
    void showEvent(QShowEvent *event);

    QPlainTextEdit *_configEdit;
    QPushButton *_saveButton;
    QLabel *_sizeLabel;
    QLabel *_checksumLabel;
    SerialConnection *_serialConnection;
};

} // namespace STMBL_Servoterm

#endif // QTSERVOTERM_CONFIGDIALOG_H
