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

#include <QtWidgets>
#include <QToolBar>
#include <QSettings>
#include <QSerialPortInfo>
#include <QTcpSocket>
#include <QHostAddress>
#include <QRegExp>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShortcut>
#include <QMessageBox>
// #include <QDebug>
#include <QDialog>

#include "MainWindow.h"
#include "Actions.h"
#include "MenuBar.h"
#include "ClickableComboBox.h"
#include "Oscilloscope.h"
#include "XYOscilloscope.h"
#include "HistoryLineEdit.h"
#include "ScopeDataDemux.h"
//#include "stmbl_config_crc32.h"

#include <limits>

namespace STMBL_Servoterm {

static const quint16 STMBL_USB_VENDOR_ID  = 0x0483; //  1155
static const quint16 STMBL_USB_PRODUCT_ID = 0x5740; // 22336

// forward declaration
template<typename T, typename M>
static void AppendTextToEdit(T &target, M insertMethod, const QString &txt);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _actions(new Actions(this)),
    _menuBar(new MenuBar(_actions, this)),
    _portList(new ClickableComboBox),
    _clearButton(new QPushButton("Clear")),
    _disableButton(new QPushButton("Disable")),
    _enableButton(new QPushButton("Enable")),
    _jogCheckbox(new QCheckBox("Jog")),
    _configButton(new QPushButton("Config")),
    _oscilloscope(new Oscilloscope),
    _xyOscilloscope(new XYOscilloscope),
    _textLog(new QTextEdit),
    _lineEdit(new HistoryLineEdit),
    _sendButton(new QPushButton("Send")),
    _serialPort(new QSerialPort(this)),
    _tcpSocket(new QTcpSocket(this)),
    _settings(new QSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName(), this)),
    _demux(new ScopeDataDemux(this)),
    _configDialog(new QDialog(this)),
    _configEdit(new QPlainTextEdit),
    _configSaveButton(new QPushButton("Save")),
    _configSizeLabel(new QLabel),
    _configChecksumLabel(new QLabel),
    _estopShortcut(new QShortcut(QKeySequence("Esc"), this)),
    _redirectingTimer(new QTimer(this)),
    _serialSendTimer(new QTimer(this)),
    _redirectingToConfigEdit(false),
    _leftPressed(false),
    _rightPressed(false)
{
    _portList->setEditable(true);
    {
        static const QString exampleIP = "xxx.xxx.xxx.xxx:yyyyy";
        QFontMetrics fm(_portList->font());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        const int fw = fm.horizontalAdvance(exampleIP);
#else
        const int fw = fm.width(exampleIP);
#endif
        _portList->setMinimumWidth(fw+30); // TODO figure out a better way of calculating a good width!
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    _portList->setPlaceholderText("ip address/port or USB device name");
#endif
    }
    _textLog->setReadOnly(true);
    {
        QTextDocument * const doc = _textLog->document();
        QFont f = doc->defaultFont();
        f.setFamily("monospace");
        f.setStyleHint(QFont::Monospace);
        doc->setDefaultFont(f);

        f = _lineEdit->font();
        f.setFamily("monospace");
        f.setStyleHint(QFont::Monospace);
        _lineEdit->setFont(f);
        _configEdit->setFont(f); // TODO this is a bit out of place, but should work just fine
        _configSizeLabel->setFont(f);
        _configChecksumLabel->setFont(f);
    }
    setAcceptDrops(true);
    qApp->installEventFilter(this);
    _configDialog->setWindowTitle("STMBL Configuration");
    _configDialog->setModal(true);
    _configSizeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _configSizeLabel->setAlignment((_configSizeLabel->alignment() & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight);
    _configSizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _configChecksumLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _configChecksumLabel->setAlignment((_configChecksumLabel->alignment() & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight);
    _configChecksumLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _redirectingTimer->setInterval(100);
    _redirectingTimer->setSingleShot(true);
    _serialSendTimer->setInterval(50);

    // TODO make these settings saved between program launches
    _actions->viewOscilloscope->setChecked(true);
    _actions->viewXYScope->setChecked(false);
    _actions->viewConsole->setChecked(true);
    // TODO find a better solution to the side effects of setVisible(true) when it's already visible but not shown yet
    if (!_actions->viewOscilloscope->isChecked())
        _oscilloscope->setVisible(_actions->viewOscilloscope->isChecked());
    if (!_actions->viewXYScope->isChecked())
        _xyOscilloscope->setVisible(_actions->viewXYScope->isChecked());
    if (!_actions->viewConsole->isChecked())
        _textLog->setVisible(_actions->viewConsole->isChecked());

    // populate config dialog
    {
        QVBoxLayout * const vbox = new QVBoxLayout(_configDialog);
        vbox->addWidget(_configEdit);

        {
            QHBoxLayout * const hbox = new QHBoxLayout;
            hbox->addWidget(_configSaveButton);
            hbox->addWidget(_configSizeLabel, 1);
            hbox->addWidget(_configChecksumLabel, 1);
            vbox->addLayout(hbox);
        }
    }

    setWindowTitle(QCoreApplication::applicationName());
    setMenuBar(_menuBar);
    {
        QToolBar * const toolbar = new QToolBar;
        toolbar->setObjectName("ConnectionToolBar");
        // toolbar->addWidget(new QPushButton("Refresh"));
        toolbar->addWidget(_portList);
        toolbar->addAction(_actions->connectionConnect);
        toolbar->addAction(_actions->connectionDisconnect);
        toolbar->addSeparator();
        toolbar->addWidget(_clearButton);
        toolbar->addSeparator();
        toolbar->addWidget(_disableButton);
        toolbar->addWidget(_enableButton);
        toolbar->addWidget(_jogCheckbox);
        toolbar->addAction(_actions->viewXYScope);
        toolbar->addWidget(_configButton);
        addToolBar(toolbar);
    }
    {
        QWidget * const dummy = new QWidget;
        QVBoxLayout * const vbox = new QVBoxLayout(dummy);
        {
            QHBoxLayout * const hbox = new QHBoxLayout;
            hbox->addWidget(_oscilloscope, 1);
            hbox->addWidget(_xyOscilloscope);
            vbox->addLayout(hbox);
        }
        vbox->addWidget(_textLog);
        {
            QHBoxLayout * const hbox = new QHBoxLayout;
            hbox->addWidget(_lineEdit);
            hbox->addWidget(_sendButton);
            vbox->addLayout(hbox);
        }
        setCentralWidget(dummy);
    }

    connect(_actions->fileQuit, &QAction::triggered, qApp, &QCoreApplication::quit, Qt::QueuedConnection);
    connect(_actions->viewOscilloscope, &QAction::toggled, _oscilloscope, &QWidget::setVisible);
    connect(_actions->viewXYScope, &QAction::toggled, _xyOscilloscope, &QWidget::setVisible);
    connect(_actions->viewConsole, &QAction::toggled, _textLog, &QWidget::setVisible);
    connect(_menuBar->portMenu, &QMenu::aboutToShow, this, &MainWindow::slot_PortListClicked);
    connect(_menuBar->portGroup, &QActionGroup::triggered, this, &MainWindow::slot_PortMenuItemSelected);
    connect(_portList, &ClickableComboBox::clicked, this, &MainWindow::slot_PortListClicked);
    connect(_portList, &ClickableComboBox::currentTextChanged, this, &MainWindow::slot_PortLineEditChanged);
    connect(_actions->connectionConnect, &QAction::triggered, this, &MainWindow::slot_ConnectClicked);
    connect(_actions->connectionDisconnect, &QAction::triggered, this, &MainWindow::slot_DisconnectClicked);
    connect(_clearButton, &QPushButton::clicked, _textLog, &QTextEdit::clear);
    connect(_disableButton, &QPushButton::clicked, this, &MainWindow::slot_DisableClicked);
    connect(_enableButton, &QPushButton::clicked, this, &MainWindow::slot_EnableClicked);
    connect(_jogCheckbox, &QCheckBox::toggled, this, &MainWindow::slot_JogToggled);
    connect(_configButton, &QPushButton::clicked, this, &MainWindow::slot_ConfigClicked);
    connect(_configSaveButton, &QPushButton::clicked, this, &MainWindow::slot_SaveClicked);
    connect(_lineEdit, &HistoryLineEdit::textChanged, this, &MainWindow::slot_UpdateButtons);
    connect(_lineEdit, &HistoryLineEdit::returnPressed, _sendButton, &QAbstractButton::click);
    connect(_sendButton, &QPushButton::clicked, this, &MainWindow::slot_SendClicked);
    connect(_serialPort, &QSerialPort::readyRead, this, &MainWindow::slot_SerialDataReceived);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    connect(_serialPort, &QSerialPort::errorOccurred, this, &MainWindow::slot_SerialErrorOccurred);
#endif
    // connect(_serialPort, &QSerialPort::readChannelFinished, this, &MainWindow::slot_SerialPortClosed); // NOTE: doesn't seem to actually work
    connect(_tcpSocket, &QTcpSocket::stateChanged, this, &MainWindow::slot_SocketStateChanged);
    connect(_tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::slot_SocketDataReceived);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(_tcpSocket, &QTcpSocket::errorOccurred, this, &MainWindow::slot_SocketErrorOccurred);
#endif
    connect(_demux, &ScopeDataDemux::scopePacketReceived, this, &MainWindow::slot_ScopePacketReceived);
    connect(_demux, &ScopeDataDemux::scopeResetReceived, this, &MainWindow::slot_ScopeResetReceived);
    connect(_textLog, &QTextEdit::textChanged, this, &MainWindow::slot_UpdateButtons);
    connect(_configEdit, &QPlainTextEdit::textChanged, this, &MainWindow::slot_ConfigTextChanged);
    connect(_estopShortcut, &QShortcut::activated, this, &MainWindow::slot_EmergencyStop);
    connect(_redirectingTimer, &QTimer::timeout, this, &MainWindow::slot_ConfigReceiveTimeout);
    connect(_serialSendTimer, &QTimer::timeout, this, &MainWindow::slot_SerialSendFromQueue);
    slot_ConfigTextChanged(); // show the initial byte count
    slot_UpdateButtons();

    _RepopulateDeviceList();
    _loadSettings();
}

MainWindow::~MainWindow()
{
    _tcpSocket->abort();
}

void MainWindow::slot_PortListClicked()
{
    _RepopulateDeviceList();
    slot_UpdateButtons();
}

void MainWindow::slot_PortLineEditChanged(const QString &portName)
{
    QList<QAction*> acts = _menuBar->portGroup->actions();
    bool found = false;
    for (QList<QAction*>::const_iterator it = acts.begin(); it != acts.end(); ++it)
    {
        QAction * const act = *it;
        if (act->text() == portName)
        {
            act->setChecked(true);
            found = true;
        }
    }
    if (!found)
    {
        QAction * const checkedAct = _menuBar->portGroup->checkedAction();
        if (checkedAct)
            checkedAct->setChecked(false);
    }
    slot_UpdateButtons();
}

void MainWindow::slot_PortMenuItemSelected(QAction *act)
{
    _portList->setCurrentText(act->text());
}

void MainWindow::slot_ConnectClicked()
{
    if (_IsConnected())
    {
        if (_serialPort->isOpen())
        {
            QMessageBox::critical(this, "Error opening serial port", "Already connected! Currently open port is: \"" + _serialPort->portName() + "\"");
        }
        else // it must be the network connection
        {
            QMessageBox::critical(this, "Error connecting to IP", "Already connected! Currently connected to: \"" + _tcpSocket->peerAddress().toString() + "\"");
        }
        return;
    }
    const QString portName = _portList->currentText();
    const bool isValidSerialPort = !QSerialPortInfo(portName).isNull();
    if (isValidSerialPort)
    {
        if (portName.isEmpty())
        {
            QMessageBox::critical(this, "Error opening serial port", "No port selected!");
            return;
        }
        _serialPort->setPortName(portName);
        if (!_serialPort->open(QIODevice::ReadWrite))
        {
            QMessageBox::critical(this, "Error opening serial port", "Unable to open port \"" + portName + "\"");
            return;
        }
        _LogConnected();
        slot_UpdateButtons();
    }
    else // must be IP (or maybe even hostname?)
    {
        const QStringList parts = portName.split(':');
        const bool isValidNetworkPort = (parts.size() == 2) && QRegExp("\\d*").exactMatch(parts.at(1)) && parts.at(1).toInt() <= std::numeric_limits<quint16>::max();
        if (!isValidNetworkPort)
        {
            QMessageBox::critical(this, "Error connecting", "Unable to interpret \"" + portName + "\" as a serial port device name nor as an IP/port");
            return;
        }
        const QString ip = parts.at(0);
        const quint16 port = parts.at(1).toInt();
        _tcpSocket->connectToHost(ip, port);
    }
}

void MainWindow::slot_DisconnectClicked()
{
    const bool wasSerialConnection = _serialPort->isOpen();
    if (_IsDisconnected())
    {
        QMessageBox::critical(this, "Error disconnecting", "Already disconnected!");
        return;
    }
    _Disconnect();
    if (!_IsDisconnected())
    {
        if (wasSerialConnection)
        {
            QMessageBox::critical(this, "Error closing serial port", "Unknown reason -- it is open, but cannot be closed?");
        }
        else if (_tcpSocket->state() != QAbstractSocket::UnconnectedState && _tcpSocket->state() != QAbstractSocket::ConnectedState)
        {
            QMessageBox::critical(this, "Error disconnecting", "a network connection is in the middle of trying to connect");
        }
        else
        {
            QMessageBox::critical(this, "Error disconnecting", "Unknown reason -- it is open, but cannot be closed?");
        }
        return;
    }
    if (wasSerialConnection)
    {
        _LogDisconnected();
    }
    slot_UpdateButtons();
}

void MainWindow::slot_EmergencyStop()
{
    _jogCheckbox->setChecked(false);
    slot_DisableClicked();
}

void MainWindow::slot_DisableClicked()
{
    if (!_IsConnected())
    {
        QMessageBox::warning(this, "Error sending reset commands", "Serial port not open!");
        return;
    }
    _SendData(QString("fault0.en = 0\n").toLatin1());
}

void MainWindow::slot_EnableClicked()
{
    if (!_IsConnected())
    {
        QMessageBox::warning(this, "Error sending reset commands", "Serial port not open!");
        return;
    }
    _SendData(QString("fault0.en = 0\n").toLatin1());
    _SendData(QString("fault0.en = 1\n").toLatin1());
}

void MainWindow::slot_JogToggled(bool on)
{
    Q_UNUSED(on);
    _DoJogging();
}

void MainWindow::slot_ConfigClicked()
{
    if (_IsConnected())
    {
        _redirectingTimer->start();
        _redirectingToConfigEdit = true;
        _configEdit->clear();
        _SendData(QString("showconf\n").toLatin1());
    }
    _configDialog->exec();
}

void MainWindow::slot_SaveClicked()
{
    if (_IsConnected())
    {
        _configDialog->hide();
        const QStringList lines = _configEdit->document()->toPlainText().split('\n', QString::KeepEmptyParts);
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
}

void MainWindow::slot_ConfigReceiveTimeout()
{
    _redirectingToConfigEdit = false;
}

void MainWindow::slot_SerialSendFromQueue()
{
    if (!_IsConnected())
        _txQueue.clear();
    if (!_txQueue.isEmpty())
        _SendData((_txQueue.takeFirst() + '\n').toLatin1());
    if (_txQueue.isEmpty())
        _serialSendTimer->stop();
}

void MainWindow::slot_SendClicked()
{
    if (!_IsConnected())
    {
        QMessageBox::warning(this, "Error sending command", "Serial port not open!");
        return;
    }
    const QString line = _lineEdit->text();
    _lineEdit->saveLine();
    _lineEdit->clear();
    _SendData((line + "\n").toLatin1()); // TODO perhaps have more intelligent Unicode conversion?
}

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

void MainWindow::slot_SerialErrorOccurred(QSerialPort::SerialPortError error)
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
        AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, QString("<font color=\"FireBrick\">serial port \"QSerialPort::") + metaEnum.valueToKey(error) + "\"</font>");
        AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
    }
    if (forceClose)
    {
        const bool before = _IsConnected();
        _Disconnect();
        const bool after = _IsConnected();
        if (before != after)
        {
            _LogDisconnected();
        }
        _RepopulateDeviceList();
        slot_UpdateButtons();
    }
    // emit serialIsConnected(false);
}

void MainWindow::slot_SerialDataReceived()
{
    _HandleReceivedData(_serialPort->readAll());
}

/*void MainWindow::slot_SerialPortClosed()
{
}*/

void MainWindow::slot_SocketStateChanged(QAbstractSocket::SocketState socketState)
{
    Q_UNUSED(socketState);
    if (socketState == QAbstractSocket::ConnectedState)
    {
        _LogConnected();
    }
    else if (socketState == QAbstractSocket::UnconnectedState)
    {
        _LogDisconnected();
    }
    slot_UpdateButtons();
}

void MainWindow::slot_SocketErrorOccurred(QAbstractSocket::SocketError error)
{
    const QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, QString("<font color=\"FireBrick\">network connection \"QAbstractSocket::") + metaEnum.valueToKey(error) + "\"</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

void MainWindow::slot_SocketDataReceived()
{
    _HandleReceivedData(_tcpSocket->readAll());
}

void MainWindow::slot_ScopePacketReceived(const QVector<float> &packet)
{
    _oscilloscope->addChannelsSample(packet);
    _xyOscilloscope->addChannelsSample(packet);
}

void MainWindow::slot_ScopeResetReceived()
{
    _oscilloscope->resetScanning();
    _xyOscilloscope->resetScanning();
}

static quint32 CalculateCRC(const QByteArray &data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);
    quint32 crc = 0xffffffff;
    while (true)
    {
        quint32 value = 0;
        stream >> value;
        if (stream.status() != QDataStream::Ok)
            break;
        crc ^= value;
        for (int j = 0; j < 32; j++)
        {
            if (crc & 0x80000000)
               crc = (crc << 1) ^ 0x04C11DB7;
            else
               crc = (crc << 1);
        }
    }

    /*const size_t len = (data.size()/4)*4;
    stmbl_config_crc32_t crc = stmbl_config_crc32_init();
    crc = stmbl_config_crc32_update(crc, data.data(), len);
    crc = stmbl_config_crc32_finalize(crc);*/
    return crc;
}

void MainWindow::slot_ConfigTextChanged()
{
    // NOTE: the -1 is to disregard an invisible
    // "paragraph separator", see the following post:
    // https://bugreports.qt.io/browse/QTBUG-4841
    _configSizeLabel->setText("Size: " + QString::number(qMax(0, _configEdit->document()->characterCount()-1)).rightJustified(6) + " bytes");
    const QByteArray configBytes = _configEdit->document()->toPlainText().toLatin1();//.replace('\n', '\r');
    const quint32 checksum = CalculateCRC(configBytes);
    _configChecksumLabel->setText("CRC: " + QString::number(checksum, 16).rightJustified(8, '0'));
}

void MainWindow::slot_UpdateButtons()
{
    const bool portSelected = !_portList->currentText().isEmpty();
    const bool portOpen = _IsConnected();
    const bool portClosed = _IsDisconnected();
    const bool hasCommand = !_lineEdit->text().isEmpty();
    _portList->setEnabled(portClosed);
    _menuBar->portGroup->setEnabled(portClosed);
    _actions->connectionConnect->setEnabled(portClosed && portSelected);
    _actions->connectionDisconnect->setEnabled(!portClosed);
    _clearButton->setEnabled(!_textLog->document()->isEmpty());
    _enableButton->setEnabled(portOpen);
    _configButton->setEnabled(portOpen);
    _sendButton->setEnabled(portOpen && hasCommand);
    _configSaveButton->setEnabled(portOpen);
}

/*void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    // check for our needed mime type, here a file or a list of files
    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        // extract the local paths of the files
        for (int i = 0; i < urlList.size() && i < 32; ++i)
        {
            pathList.append(urlList.at(i).toLocalFile());
        }

        // call a function to open the files
        qDebug() << pathList;
        // openFiles(pathList);
    }
}*/

void MainWindow::closeEvent(QCloseEvent *event)
{
    _saveSettings();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease))
    {
        QKeyEvent * const keyEvent = static_cast<QKeyEvent*>(event);
        const bool pressed = (event->type() == QEvent::KeyPress);
        if (keyEvent->key() == Qt::Key_Left)
            _leftPressed = pressed;
        else if (keyEvent->key() == Qt::Key_Right)
            _rightPressed = pressed;
        else
            return QObject::eventFilter(obj, event);
        _DoJogging();
        return _jogCheckbox->isChecked();
    }
    return QObject::eventFilter(obj, event);
}

void MainWindow::_LogConnected()
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">connected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

void MainWindow::_LogDisconnected()
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">disconnected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

bool MainWindow::_IsConnected() const
{
    // return _serialPort->isOpen() || _tcpSocket->isOpen();
    return _serialPort->isOpen() || _tcpSocket->state() == QAbstractSocket::ConnectedState;
}

bool MainWindow::_IsDisconnected() const
{
    // return !_serialPort->isOpen() && !_tcpSocket->isOpen();
    return !_serialPort->isOpen() && _tcpSocket->state() == QAbstractSocket::UnconnectedState;
}

void MainWindow::_Disconnect()
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

void MainWindow::_HandleReceivedData(const QByteArray &data)
{
    const QString txt = _demux->addData(data).replace("<=", "&lt;="); // HACK, we really need to stop interpreting the data as HTML...
    if (!txt.isEmpty())
    {
        if (_redirectingToConfigEdit)
        {
            _redirectingTimer->start(); // extend (restart) timer to delay timeout
            AppendTextToEdit(*_configEdit, &QPlainTextEdit::insertPlainText, txt);
        }
        else
            AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, txt);
    }
}

void MainWindow::_SendData(const QByteArray &data)
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

void MainWindow::_DoJogging()
{
    if (!_IsConnected())
    {
        // _jogState = JOGGING_IDLE; // TODO set to an invalid value to force sending a sychronizing command after (re)connecting
        return;
    }

    // determine new state
    JogState newState = JOGGING_IDLE;
    if (_jogCheckbox->isChecked())
    {
        if (_leftPressed && !_rightPressed)
            newState = JOGGING_CCW;
        else if (_rightPressed && !_leftPressed)
            newState = JOGGING_CW;
    }

    // synchronize the STMBL drive to the GUI's jog state
    if (newState != _jogState)
    {
        _jogState = newState;
        switch (_jogState)
        {
            case JOGGING_CCW:
            _SendData("jogl\n");
            break;

            case JOGGING_CW:
            _SendData("jogr\n");
            break;

            default:
            _SendData("jogx\n");
        }
    }
}

void MainWindow::_RepopulateDeviceList()
{
    // build new list of ports
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    QStringList portNames;
    for (QList<QSerialPortInfo>::const_iterator it = ports.begin(); it != ports.end(); ++it)
    {
        if (it->manufacturer().contains("STMicroelectronics")
         || it->description().contains("STMBL")
         || (it->vendorIdentifier() == STMBL_USB_VENDOR_ID
          && it->productIdentifier()== STMBL_USB_PRODUCT_ID))
        {
            portNames.append(it->portName());
        }
    }

    // build old list of ports
    QStringList oldPortNames;
    for (int i = 0; i < _portList->count(); i++)
    {
        oldPortNames.append(_portList->itemText(i));
    }

    // possibly add fake ports for testing/development purposes
    if (0)
    {
        QStringList fakePorts = {"/dev/ttyUSB0", "/dev/ttyACM0", "/dev/ttyS0"};
        portNames += fakePorts;
    }

    // nothing changed, exit early
    if (portNames == oldPortNames)
        return;

    // remember the currently selected port so we can reselect it
    const QString oldPortName = _portList->currentText();

    // rebuild the user interface items for the selectable ports
    _portList->clear();
    _menuBar->portMenu->clear();
    for (QStringList::const_iterator it = portNames.begin(); it != portNames.end(); ++it)
    {
        // NOTE: the combo-box entry must be added after the menu
        // bar entry due to the way the menu bar reacts to changes
        // in the line edit
        const QString portName = *it;
        QAction * const act = _menuBar->portMenu->addAction(portName);
        act->setCheckable(true);
        _menuBar->portGroup->addAction(act);
        _portList->addItem(portName);
        if (portName == oldPortName)
        {
            act->setChecked(true);
            _portList->setCurrentIndex(_portList->count()-1);
        }
    }
}

void MainWindow::_saveSettings()
{
    _settings->beginGroup("MainWindow");
    _settings->setValue("geometry", saveGeometry());
    _settings->setValue("windowState", saveState());
    _settings->endGroup();
    _settings->beginGroup("ConfigDialog");
    _settings->setValue("geometry", _configDialog->saveGeometry());
    _settings->endGroup();
}

void MainWindow::_loadSettings()
{
    _settings->beginGroup("MainWindow");
    restoreGeometry(_settings->value("geometry").toByteArray());
    restoreState(_settings->value("windowState").toByteArray());
    _settings->endGroup();
    _settings->beginGroup("ConfigDialog");
    _configDialog->restoreGeometry(_settings->value("geometry").toByteArray());
    _settings->endGroup();
}

} // namespace STMBL_Servoterm
