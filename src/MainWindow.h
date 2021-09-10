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

#ifndef STMBL_SERVOTERM_MAINWINDOW_H
#define STMBL_SERVOTERM_MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QPushButton;
class QCheckBox;
class QTextEdit;
class QShortcut;
class QSettings;
class QTimer;
class QFile;
QT_END_NAMESPACE

namespace STMBL_Servoterm {

class Actions;
class MenuBar;
class ClickableComboBox;
class ConfigDialog;
class Oscilloscope;
class XYOscilloscope;
class HistoryLineEdit;
class SerialConnection;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected slots:
    void slot_PortListClicked();
    void slot_PortLineEditChanged(const QString &portName);
    void slot_PortMenuItemSelected(QAction *act);
    void slot_ConnectClicked();
    void slot_DisconnectClicked();
    void slot_EmergencyStop();
    void slot_DisableClicked();
    void slot_EnableClicked();
    void slot_DataRecordToggled(bool recording);
    void slot_DataSetDirectoryClicked();
    void slot_DataOpenDirectoryClicked();
    void slot_SendClicked();
    void slot_SerialConnected();
    void slot_SerialDisconnected();
    void slot_LogLine(const QString &line);
    void slot_LogError(const QString &errorMessage);
    void slot_ScopePacketReceived(const QVector<float> &packet);
    void slot_ScopeResetReceived();
    void slot_UpdateButtons();
    void slot_SendJogCommand();
protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void _RepopulateDeviceList();
    void _saveSettings();
    void _loadSettings();

    SerialConnection *_serialConnection;
    Actions *_actions;
    MenuBar *_menuBar;
    ClickableComboBox *_portList;
    Oscilloscope *_oscilloscope;
    XYOscilloscope *_xyOscilloscope;
    QTextEdit *_textLog;
    HistoryLineEdit *_lineEdit;
    QPushButton *_sendButton;
    QSettings *_settings;
    ConfigDialog *_configDialog;
    QTimer *_jogTimer;
    QFile *_csvFile;
    
    QShortcut *_estopShortcut;
    bool _leftPressed;
    bool _rightPressed;
    enum JogState
    {
        JOGGING_IDLE,
        JOGGING_CW,
        JOGGING_CCW
    } _jogState;
    QString _recordingsDirectory;
};

} // namespace STMBL_Servoterm

#endif // QTSERVOTERM_MAINWINDOW_H
