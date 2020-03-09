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
#include <QChartGlobal>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QComboBox;
class QPushButton;
class QTextEdit;
class QPlainTextEdit;
class QLabel;
class QSerialPort;
class QSettings;
QT_END_NAMESPACE

QT_CHARTS_BEGIN_NAMESPACE
class QChartView;
class QChart;
class QLineSeries;
QT_CHARTS_END_NAMESPACE

namespace STMBL_Servoterm {

class HistoryLineEdit;
class ScopeDataDemux;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected slots:
    void slot_ConnectClicked();
    void slot_DisconnectClicked();
    void slot_ResetClicked();
    void slot_ConfigClicked();
    void slot_SaveClicked();
    void slot_ConfigReceiveTimeout();
    void slot_SerialSendFromQueue();
    void slot_SendClicked();
    void slot_SerialDataReceived();
    // void slot_SerialPortClosed();
    void slot_ScopePacketReceived(const QVector<float> &packet);
    void slot_ScopeResetReceived();
    void slot_ConfigTextChanged();
    void slot_UpdateButtons();
protected:
    /*void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);*/
    void closeEvent(QCloseEvent *event);
    void _RepopulateDeviceList();
    void _saveSettings();
    void _loadSettings();

    static const int SCOPE_CHANNEL_COUNT = 8;

    QComboBox *_portList;
    QPushButton *_connectButton;
    QPushButton *_disconnectButton;
    QPushButton *_clearButton;
    QPushButton *_resetButton;
    QPushButton *_configButton;
    QT_CHARTS_NAMESPACE::QChartView *_chartView;
    QT_CHARTS_NAMESPACE::QChart *_chart;
    QT_CHARTS_NAMESPACE::QLineSeries *_chartData[SCOPE_CHANNEL_COUNT];
    QT_CHARTS_NAMESPACE::QLineSeries *_chartRollingLine;
    QTextEdit *_textLog;
    HistoryLineEdit *_lineEdit;
    QPushButton *_sendButton;
    QSerialPort *_serialPort;
    QSettings *_settings;
    ScopeDataDemux *_demux;
    QDialog *_configDialog;
    QPlainTextEdit *_configEdit;
    QPushButton *_configSaveButton;
    QLabel *_configSizeLabel;
    QTimer *_redirectingTimer;
    QTimer *_serialSendTimer;
	QStringList _txQueue;
    int _scopeX;
    bool _redirectingToConfigEdit;
};

} // namespace STMBL_Servoterm

#endif // QTSERVOTERM_MAINWINDOW_H
