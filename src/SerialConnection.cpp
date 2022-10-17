#include "SerialConnection.h"
#include "ScopeDataDemux.h"

#include <QSerialPortInfo>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QMessageBox>
#include <QMetaEnum>

namespace STMBL_Servoterm {

static const quint16 STMBL_USB_VENDOR_ID  = 0x0483; //  1155
static const quint16 STMBL_USB_PRODUCT_ID = 0x5740; // 22336

SerialConnection::SerialConnection(QObject *parent) :
    QObject(parent),
    _serialPort(new QSerialPort(this)),
    _tcpSocket(new QTcpSocket(this)),
    _demux(new ScopeDataDemux(this)),
    _redirectingTimer(new QTimer(this)),
    _serialSendTimer(new QTimer(this)),
    _redirectingToConfigEdit(false)
{
    _redirectingTimer->setInterval(100);
    _redirectingTimer->setSingleShot(true);
    _serialSendTimer->setInterval(50);

    connect(_serialPort, &QSerialPort::readyRead, this, &SerialConnection::slot_SerialDataReceived);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    connect(_serialPort, &QSerialPort::errorOccurred, this, &SerialConnection::slot_SerialErrorOccurred);
#endif
    // connect(_serialPort, &QSerialPort::readChannelFinished, this, &SerialConnection::slot_SerialPortClosed); // NOTE: doesn't seem to actually work
    connect(_tcpSocket, &QTcpSocket::stateChanged, this, &SerialConnection::slot_SocketStateChanged);
    connect(_tcpSocket, &QTcpSocket::readyRead, this, &SerialConnection::slot_SocketDataReceived);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(_tcpSocket, &QTcpSocket::errorOccurred, this, &SerialConnection::slot_SocketErrorOccurred);
#endif
    connect(_demux, &ScopeDataDemux::scopePacketReceived, this, &SerialConnection::scopePacketReceived);
    connect(_demux, &ScopeDataDemux::scopeResetReceived, this, &SerialConnection::scopeResetReceived);
    connect(_redirectingTimer, &QTimer::timeout, this, &SerialConnection::slot_ConfigReceiveTimeout);
    connect(_serialSendTimer, &QTimer::timeout, this, &SerialConnection::slot_SerialSendFromQueue);
}

SerialConnection::~SerialConnection()
{
    _tcpSocket->abort();
}

QStringList SerialConnection::getSerialPortNames()
{
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    QStringList portNames;
    for (QList<QSerialPortInfo>::const_iterator it = ports.begin(); it != ports.end(); ++it)
    {
        /*if (it->manufacturer().contains("STMicroelectronics")
         || it->description().contains("STMBL")
         || (it->vendorIdentifier() == STMBL_USB_VENDOR_ID
          && it->productIdentifier()== STMBL_USB_PRODUCT_ID))*/
        {
            portNames.append(it->portName());
        }
    }
    // possibly add fake ports for testing/development purposes
    if (0)
    {
        QStringList fakePorts = {"/dev/ttyUSB0", "/dev/ttyACM0", "/dev/ttyS0"};
        portNames += fakePorts;
    }
    return portNames;
}

bool SerialConnection::isValidSerialPortName(const QString &portName)
{
    return QSerialPortInfo(portName).isNull();
}

bool SerialConnection::isSerialConnection() const
{
    return _serialPort->isOpen();
}

bool SerialConnection::isConnected() const
{
    // return _serialPort->isOpen() || _tcpSocket->isOpen();
    return _serialPort->isOpen() || _tcpSocket->state() == QAbstractSocket::ConnectedState;
}

bool SerialConnection::isDisconnected() const
{
    // return !_serialPort->isOpen() && !_tcpSocket->isOpen();
    return !_serialPort->isOpen() && _tcpSocket->state() == QAbstractSocket::UnconnectedState;
}

QString SerialConnection::serialPortName() const
{
    return _serialPort->portName();
}

QString SerialConnection::networkPeerAddress() const
{
    return _tcpSocket->peerAddress().toString();
}

void SerialConnection::connectTo(const QString &portName)
{
    if (isConnected())
    {
        if (isSerialConnection())
        {
            QMessageBox::critical(nullptr, "Error opening serial port", "Already connected! Currently open port is: \"" + serialPortName() + "\"");
        }
        else // it must be the network connection
        {
            QMessageBox::critical(nullptr, "Error connecting to IP", "Already connected! Currently connected to: \"" + networkPeerAddress() + "\"");
        }
        return;
    }
    const bool isValidSerialPort = !isValidSerialPortName(portName);
    if (isValidSerialPort)
    {
        if (portName.isEmpty())
        {
            QMessageBox::critical(nullptr, "Error opening serial port", "No port selected!");
            return;
        }
        _serialPort->setPortName(portName);
        _serialPort->setBaudRate(115200);
        if (!_serialPort->open(QIODevice::ReadWrite))
        {
            QMessageBox::critical(nullptr, "Error opening serial port", "Unable to open port \"" + portName + "\"");
            return;
        }
        emit connected();
    }
    else // must be IP (or maybe even hostname?)
    {
        const QStringList parts = portName.split(':');
        const bool isValidNetworkPort = (parts.size() == 2) && QRegExp("\\d*").exactMatch(parts.at(1)) && parts.at(1).toInt() <= std::numeric_limits<quint16>::max();
        if (!isValidNetworkPort)
        {
            QMessageBox::critical(nullptr, "Error connecting", "Unable to interpret \"" + portName + "\" as a serial port device name nor as an IP/port");
            return;
        }
        const QString ip = parts.at(0);
        const quint16 port = parts.at(1).toInt();
        _tcpSocket->connectToHost(ip, port);
    }
}

void SerialConnection::disconnectFrom()
{
    const bool wasSerialConnection = isSerialConnection();
    if (isDisconnected())
    {
        QMessageBox::critical(nullptr, "Error disconnecting", "Already disconnected!");
        return;
    }
    _Disconnect();
    if (!isDisconnected())
    {
        if (wasSerialConnection)
        {
            QMessageBox::critical(nullptr, "Error closing serial port", "Unknown reason -- it is open, but cannot be closed?");
        }
        else if (_tcpSocket->state() != QAbstractSocket::UnconnectedState && _tcpSocket->state() != QAbstractSocket::ConnectedState)
        {
            QMessageBox::critical(nullptr, "Error disconnecting", "a network connection is in the middle of trying to connect");
        }
        else
        {
            QMessageBox::critical(nullptr, "Error disconnecting", "Unknown reason -- it is open, but cannot be closed?");
        }
        return;
    }
    if (wasSerialConnection)
    {
        emit disconnected();
    }
}

void SerialConnection::sendData(const QByteArray &data)
{
    if (_serialPort->isOpen())
    {
        _serialPort->write(data);
    }
    else if (_tcpSocket->isOpen())
    {
        _tcpSocket->write(data);
    }
}

void SerialConnection::sendConfig(const QString &config)
{
    const QStringList lines = config.split('\n',
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        Qt::KeepEmptyParts);
#else
        QString::KeepEmptyParts);
#endif
    _txQueue.clear();
    _txQueue.append("deleteconf");
    for (QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it)
    {
        _txQueue.append(QString("appendconf ") + *it);
    }
    _txQueue.append("flashsaveconf");
    _serialSendTimer->start();
    slot_SerialSendFromQueue();
}

void SerialConnection::startReadingConfig()
{
    _redirectingToConfigEdit = true;
    _redirectingTimer->start();
    sendData(QString("showconf\n").toLatin1());
}

void SerialConnection::slot_ConfigReceiveTimeout()
{
    _redirectingToConfigEdit = false;
}

void SerialConnection::slot_SerialSendFromQueue()
{
    if (!isConnected())
        _txQueue.clear();
    if (!_txQueue.isEmpty())
        sendData((_txQueue.takeFirst() + '\n').toLatin1());
    if (_txQueue.isEmpty())
        _serialSendTimer->stop();
}

void SerialConnection::slot_SerialErrorOccurred(QSerialPort::SerialPortError error)
{
    QString errorMsg;
    bool forceClose = false;
    switch (error)
    {
        case QSerialPort::NoError:
        return; // all good!

        // case QSerialPort::DeviceNotFoundError:
        // case QSerialPort::PermissionError:
        // case QSerialPort::OpenError:
        // case QSerialPort::NotOpenError:

        case QSerialPort::WriteError:
        errorMsg = "serial port write error";
        forceClose = true;
        break;

        case QSerialPort::ReadError:
        errorMsg = "serial port read error";
        forceClose = true;
        break;

        case QSerialPort::ResourceError:
        errorMsg = "serial port resource error";
        forceClose = true;
        break;

        // case QSerialPort::UnsupportedOperationError:
        // case QSerialPort::TimeoutError:
        // case QSerialPort::UnknownError:
        default:
        break;
    }
    if (errorMsg.isEmpty())
    {
        const QMetaEnum metaEnum = QMetaEnum::fromType<QSerialPort::SerialPortError>();
        emit errorMessage(QString("serial port \"QSerialPort::") + metaEnum.valueToKey(error) + "\"");
    }
    if (forceClose)
    {
        const bool before = isConnected();
        _Disconnect();
        const bool after = isConnected();
        if (before != after)
        {
            emit disconnected();
        }
    }
}

void SerialConnection::slot_SerialDataReceived()
{
    _HandleReceivedData(_serialPort->readAll());
}

/*void SerialConnection::slot_SerialPortClosed()
{
}*/

void SerialConnection::slot_SocketStateChanged(QAbstractSocket::SocketState socketState)
{
    if (socketState == QAbstractSocket::ConnectedState)
    {
        emit connected();
    }
    else if (socketState == QAbstractSocket::UnconnectedState)
    {
        emit disconnected();
    }
}

void SerialConnection::slot_SocketErrorOccurred(QAbstractSocket::SocketError error)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
    emit errorMessage(QString("network connection \"QAbstractSocket::") + metaEnum.valueToKey(error) + "\"");
}

void SerialConnection::slot_SocketDataReceived()
{
    _HandleReceivedData(_tcpSocket->readAll());
}

void SerialConnection::_Disconnect()
{
    if (_serialPort->isOpen())
    {
        _serialPort->close();
    }
    else // it must be the network connection
    {
        _tcpSocket->abort(); // close() and disconnectFromHost() were tried before
    }
}

void SerialConnection::_HandleReceivedData(const QByteArray &data)
{
    QString txt = _demux->addData(data);
    if (!txt.isEmpty())
    {
        if (_redirectingToConfigEdit)
        {
            _redirectingTimer->start(); // extend (restart) timer to delay timeout
            emit configLineReceived(txt);
        }
        else
            emit lineReceived(txt.replace("<=", "&lt;=")); // HACK, we really need to stop interpreting the data as HTML...
    }
}

} // namespace STMBL_Servoterm

