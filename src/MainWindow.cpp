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
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
// #include <QDebug>
#include <QDialog>

#include "MainWindow.h"
#include "ClickableComboBox.h"
#include "Oscilloscope.h"
#include "XYOscilloscope.h"
#include "HistoryLineEdit.h"
#include "ScopeDataDemux.h"

namespace STMBL_Servoterm {

static const quint16 STMBL_USB_VENDOR_ID  = 0x0483; //  1155
static const quint16 STMBL_USB_PRODUCT_ID = 0x5740; // 22336

// forward declaration
template<typename T, typename M>
static void AppendTextToEdit(T &target, M insertMethod, const QString &txt);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _portList(new ClickableComboBox),
    _connectButton(new QPushButton("Connect")),
    _disconnectButton(new QPushButton("Disconnect")),
    _clearButton(new QPushButton("Clear")),
    _resetButton(new QPushButton("Reset")),
    _jogCheckbox(new QCheckBox("Jog")),
    _xyCheckbox(new QCheckBox("XY Mode")),
    _configButton(new QPushButton("Config")),
    _oscilloscope(new Oscilloscope),
    _xyOscilloscope(new XYOscilloscope),
    _textLog(new QTextEdit),
    _lineEdit(new HistoryLineEdit),
    _sendButton(new QPushButton("Send")),
    _serialPort(new QSerialPort(this)),
    _settings(nullptr),
    _demux(new ScopeDataDemux(this)),
    _configDialog(new QDialog(this)),
    _configEdit(new QPlainTextEdit),
    _configSaveButton(new QPushButton("Save")),
    _configSizeLabel(new QLabel),
    _redirectingTimer(new QTimer(this)),
    _serialSendTimer(new QTimer(this)),
    _redirectingToConfigEdit(false),
    _leftPressed(false),
    _rightPressed(false)
{
    _settings = new QSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
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
    }
    setAcceptDrops(true);
    qApp->installEventFilter(this);
    _configDialog->setWindowTitle("STMBL Configuration");
    _configDialog->setModal(true);
    _configSizeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _configSizeLabel->setAlignment((_configSizeLabel->alignment() & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight);
    _configSizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _redirectingTimer->setInterval(100);
    _redirectingTimer->setSingleShot(true);
    _serialSendTimer->setInterval(50);
    _xyCheckbox->setChecked(false); // TODO make this a saved setting
    _xyOscilloscope->setVisible(_xyCheckbox->isChecked());

    // populate config dialog
    {
        QVBoxLayout * const vbox = new QVBoxLayout(_configDialog);
        vbox->addWidget(_configEdit);

        {
            QHBoxLayout * const hbox = new QHBoxLayout;
            hbox->addWidget(_configSaveButton);
            hbox->addWidget(_configSizeLabel, 1);
            vbox->addLayout(hbox);
        }
    }

    setWindowTitle(QCoreApplication::applicationName());
    {
        QToolBar * const toolbar = new QToolBar;
        toolbar->setObjectName("ConnectionToolBar");
        // toolbar->addWidget(new QPushButton("Refresh"));
        toolbar->addWidget(_portList);
        toolbar->addWidget(_connectButton);
        toolbar->addWidget(_disconnectButton);
        toolbar->addSeparator();
        toolbar->addWidget(_clearButton);
        toolbar->addWidget(_resetButton);
        toolbar->addWidget(_jogCheckbox);
        toolbar->addWidget(_xyCheckbox);
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

    connect(_portList, SIGNAL(clicked()), this, SLOT(slot_PortListClicked()));
    connect(_portList, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(slot_UpdateButtons()));
    connect(_connectButton, SIGNAL(clicked()), this, SLOT(slot_ConnectClicked()));
    connect(_disconnectButton, SIGNAL(clicked()), this, SLOT(slot_DisconnectClicked()));
    connect(_clearButton, SIGNAL(clicked()), _textLog, SLOT(clear()));
    connect(_resetButton, SIGNAL(clicked()), this, SLOT(slot_ResetClicked()));
    connect(_jogCheckbox, SIGNAL(toggled(bool)), this, SLOT(slot_JogToggled(bool)));
    connect(_xyCheckbox, SIGNAL(toggled(bool)), _xyOscilloscope, SLOT(setVisible(bool)));
    connect(_configButton, SIGNAL(clicked()), this, SLOT(slot_ConfigClicked()));
    connect(_configSaveButton, SIGNAL(clicked()), this, SLOT(slot_SaveClicked()));
    connect(_lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slot_UpdateButtons()));
    connect(_lineEdit, SIGNAL(returnPressed()), _sendButton, SLOT(click()));
    connect(_sendButton, SIGNAL(clicked()), this, SLOT(slot_SendClicked()));
    connect(_serialPort, SIGNAL(readyRead()), this, SLOT(slot_SerialDataReceived()));
    connect(_serialPort, SIGNAL(errorOccurred(QSerialPort::SerialPortError)), this, SLOT(slot_SerialErrorOccurred(QSerialPort::SerialPortError)));
    // connect(_serialPort, SIGNAL(readChannelFinished()), this, SLOT(slot_SerialPortClosed())); // NOTE: doesn't seem to actually work
    connect(_demux, SIGNAL(scopePacketReceived(const QVector<float> &)), this, SLOT(slot_ScopePacketReceived(const QVector<float> &)));
    connect(_demux, SIGNAL(scopeResetReceived()), this, SLOT(slot_ScopeResetReceived()));
    connect(_textLog, SIGNAL(textChanged()), this, SLOT(slot_UpdateButtons()));
    connect(_configEdit, SIGNAL(textChanged()), this, SLOT(slot_ConfigTextChanged()));
    connect(_redirectingTimer, SIGNAL(timeout()), this, SLOT(slot_ConfigReceiveTimeout()));
    connect(_serialSendTimer, SIGNAL(timeout()), this, SLOT(slot_SerialSendFromQueue()));
    slot_ConfigTextChanged(); // show the initial byte count
    slot_UpdateButtons();

    _RepopulateDeviceList();
    _loadSettings();
}

MainWindow::~MainWindow()
{
}

void MainWindow::slot_PortListClicked()
{
    _RepopulateDeviceList();
    slot_UpdateButtons();
}

void MainWindow::slot_ConnectClicked()
{
    if (_serialPort->isOpen())
    {
        QMessageBox::critical(this, "Error opening serial port", "Already connected! Currently open port is: \"" + _serialPort->portName() + "\"");
        return;
    }
    const QString portName = _portList->currentText();
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
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">connected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
    slot_UpdateButtons();
}

void MainWindow::slot_DisconnectClicked()
{
    if (!_serialPort->isOpen())
    {
        QMessageBox::critical(this, "Error closing serial port", "Already disconnected!");
        return;
    }
    _serialPort->close();
    if (_serialPort->isOpen())
    {
        QMessageBox::critical(this, "Error closing serial port", "Unknown reason -- it is open, but cannot be closed?");
        return;
    }
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">disconnected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
    slot_UpdateButtons();
}

void MainWindow::slot_ResetClicked()
{
    if (!_serialPort->isOpen())
    {
        QMessageBox::warning(this, "Error sending reset commands", "Serial port not open!");
        return;
    }
    _serialPort->write(QString("fault0.en = 0\n").toLatin1());
    _serialPort->write(QString("fault0.en = 1\n").toLatin1());
}

void MainWindow::slot_JogToggled(bool on)
{
    Q_UNUSED(on);
    _DoJogging();
}

void MainWindow::slot_ConfigClicked()
{
    if (_serialPort->isOpen())
    {
        _redirectingTimer->start();
        _redirectingToConfigEdit = true;
        _configEdit->clear();
        _serialPort->write(QString("showconf\n").toLatin1());
    }
    _configDialog->exec();
}

void MainWindow::slot_SaveClicked()
{
    if (_serialPort->isOpen())
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
    if (!_serialPort->isOpen())
        _txQueue.clear();
    if (!_txQueue.isEmpty())
        _serialPort->write((_txQueue.takeFirst() + '\n').toLatin1());
    if (_txQueue.isEmpty())
        _serialSendTimer->stop();
}

void MainWindow::slot_SendClicked()
{
    if (!_serialPort->isOpen())
    {
        QMessageBox::warning(this, "Error sending command", "Serial port not open!");
        return;
    }
    const QString line = _lineEdit->text();
    _lineEdit->saveLine();
    _lineEdit->clear();
    _serialPort->write((line + "\n").toLatin1()); // TODO perhaps have more intelligent Unicode conversion?
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
        QMetaEnum metaEnum = QMetaEnum::fromType<QSerialPort::SerialPortError>();
        AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, QString("<font color=\"FireBrick\">serial port \"QSerialPort::") + metaEnum.valueToKey(error) + "\"</font>");
        AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
    }
    if (forceClose)
    {
        const bool before = _serialPort->isOpen();
        _serialPort->close();
        const bool after = _serialPort->isOpen();
        if (before != after)
        {
            AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">disconnected</font>");
            AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
        }
        _RepopulateDeviceList();
        slot_UpdateButtons();
    }
    // emit serialIsConnected(false);
}

void MainWindow::slot_SerialDataReceived()
{
    const QByteArray buf = _serialPort->readAll();
    const QString txt = _demux->addData(buf).replace("<=", "&lt;="); // HACK, we really need to stop interpreting the data as HTML...
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

/*void MainWindow::slot_SerialPortClosed()
{
}*/

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

void MainWindow::slot_ConfigTextChanged()
{
    // NOTE: the -1 is to disregard an invisible
    // "paragraph separator", see the following post:
    // https://bugreports.qt.io/browse/QTBUG-4841
    _configSizeLabel->setText(QString::number(qMax(0, _configEdit->document()->characterCount()-1)) + " bytes");
}

void MainWindow::slot_UpdateButtons()
{
    const bool portSelected = !_portList->currentText().isEmpty();
    const bool portOpen = _serialPort->isOpen();
    const bool hasCommand = !_lineEdit->text().isEmpty();
    _portList->setEnabled(!portOpen || !portSelected);
    _connectButton->setEnabled(!portOpen && portSelected);
    _disconnectButton->setEnabled(portOpen);
    _clearButton->setEnabled(!_textLog->document()->isEmpty());
    _resetButton->setEnabled(portOpen);
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

void MainWindow::_DoJogging()
{
    if (!_serialPort->isOpen())
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
            _serialPort->write("jogl\n");
            break;

            case JOGGING_CW:
            _serialPort->write("jogr\n");
            break;

            default:
            _serialPort->write("jogx\n");
        }
    }
}

void MainWindow::_RepopulateDeviceList()
{
    _portList->clear();
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (QList<QSerialPortInfo>::const_iterator it = ports.begin(); it != ports.end(); ++it)
    {
        if (it->manufacturer().contains("STMicroelectronics")
         || it->description().contains("STMBL")
         || (it->vendorIdentifier() == STMBL_USB_VENDOR_ID
          && it->productIdentifier()== STMBL_USB_PRODUCT_ID))
        {
            _portList->addItem(it->portName());
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
