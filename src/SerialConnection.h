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

#ifndef QTSERVOTERM_SERIALCONNECTION_H
#define QTSERVOTERM_SERIALCONNECTION_H

#include <QObject>
#include <QSerialPort>
#include <QAbstractSocket>

QT_BEGIN_NAMESPACE
class QTcpSocket;
class QTimer;
QT_END_NAMESPACE

namespace STMBL_Servoterm {

class ScopeDataDemux;

class SerialConnection : public QObject
{
    Q_OBJECT
public:
    SerialConnection(QObject *parent = nullptr);
    ~SerialConnection();

    static QStringList getSerialPortNames();
    static bool isValidSerialPortName(const QString &portName);
    bool isSerialConnection() const;
    bool isConnected() const;
    bool isDisconnected() const;
    QString serialPortName() const;
    QString networkPeerAddress() const;

    void connectTo(const QString &portName);
    void disconnectFrom();
    void sendData(const QByteArray &data);
    void startReadingConfig();
signals:
    void lineReceived(const QString &line);
    void configLineReceived(const QString &line);
    void scopePacketReceived(const QVector<float> &packet);
    void scopeResetReceived();
    void connected();
    void disconnected();
    void errorMessage(const QString &errorMessage);
protected slots:
    void slot_ConfigReceiveTimeout();
    void slot_SerialSendFromQueue();
    void slot_SerialErrorOccurred(QSerialPort::SerialPortError error);
    void slot_SerialDataReceived();
    // void slot_SerialPortClosed();
    void slot_SocketStateChanged(QAbstractSocket::SocketState socketState);
    void slot_SocketErrorOccurred(QAbstractSocket::SocketError error);
    void slot_SocketDataReceived();
protected:
    void _Disconnect();
    void _HandleReceivedData(const QByteArray &data);

    QSerialPort *_serialPort;
    QTcpSocket *_tcpSocket;
    ScopeDataDemux *_demux;
    QTimer *_redirectingTimer;
    QTimer *_serialSendTimer;
    QStringList _txQueue;
    bool _redirectingToConfigEdit;
};

} // namespace STMBL_Servoterm

#endif // QTSERVOTERM_SERIALCONNECTION_H
