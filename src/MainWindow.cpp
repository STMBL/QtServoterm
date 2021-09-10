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
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QDesktopServices>
// #include <QDebug>

#include "MainWindow.h"
#include "AppendTextToEdit.h"
#include "Actions.h"
#include "MenuBar.h"
#include "ClickableComboBox.h"
#include "ConfigDialog.h"
#include "Oscilloscope.h"
#include "XYOscilloscope.h"
#include "HistoryLineEdit.h"
#include "SerialConnection.h"

#include <limits>

namespace STMBL_Servoterm {

// forward declaration
template<typename T, typename M>
static void AppendTextToEdit(T &target, M insertMethod, const QString &txt);

static const int SEND_JOG_COMMAND_PERIOD_MS = 250; // 250ms, which is earlier than the 750ms timeout on the STMBL drive

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _serialConnection(new SerialConnection(this)),
    _actions(new Actions(this)),
    _menuBar(new MenuBar(_actions, this)),
    _portList(new ClickableComboBox),
    _oscilloscope(new Oscilloscope),
    _xyOscilloscope(new XYOscilloscope),
    _textLog(new QTextEdit),
    _lineEdit(new HistoryLineEdit),
    _sendButton(new QPushButton("Send")),
    _settings(new QSettings(QCoreApplication::organizationName(), QCoreApplication::applicationName(), this)),
    _configDialog(new ConfigDialog(_serialConnection, this)),
    _jogTimer(new QTimer(this)),
    _csvFile(new QFile(this)),
    _estopShortcut(new QShortcut(QKeySequence("Esc"), this)),
    _leftPressed(false),
    _rightPressed(false)
{
    _jogTimer->setInterval(SEND_JOG_COMMAND_PERIOD_MS);
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
    }
    setAcceptDrops(true);
    qApp->installEventFilter(this);

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
        toolbar->addAction(_actions->viewClearConsole);
        toolbar->addSeparator();
        toolbar->addAction(_actions->driveDisable);
        toolbar->addAction(_actions->driveEnable);
        toolbar->addAction(_actions->driveJogEnable);
        toolbar->addAction(_actions->viewXYScope);
        toolbar->addAction(_actions->driveEditConfig);
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
    connect(_actions->viewClearConsole, &QAction::triggered, _textLog, &QTextEdit::clear);
    connect(_actions->driveDisable, &QAction::triggered, this, &MainWindow::slot_DisableClicked);
    connect(_actions->driveEnable, &QAction::triggered, this, &MainWindow::slot_EnableClicked);
    connect(_actions->driveJogEnable, &QAction::toggled, this, &MainWindow::slot_SendJogCommand);
    connect(_actions->driveEditConfig, &QAction::triggered, _configDialog, &ConfigDialog::exec);
    connect(_actions->dataRecord, &QAction::toggled, this, &MainWindow::slot_DataRecordToggled);
    connect(_actions->dataSetDirectory, &QAction::triggered, this, &MainWindow::slot_DataSetDirectoryClicked);
    connect(_actions->dataOpenDirectory, &QAction::triggered, this, &MainWindow::slot_DataOpenDirectoryClicked);
    connect(_lineEdit, &HistoryLineEdit::textChanged, this, &MainWindow::slot_UpdateButtons);
    connect(_lineEdit, &HistoryLineEdit::returnPressed, _sendButton, &QAbstractButton::click);
    connect(_sendButton, &QPushButton::clicked, this, &MainWindow::slot_SendClicked);
    connect(_textLog, &QTextEdit::textChanged, this, &MainWindow::slot_UpdateButtons);
    connect(_estopShortcut, &QShortcut::activated, this, &MainWindow::slot_EmergencyStop);
    connect(_serialConnection, &SerialConnection::lineReceived, this, &MainWindow::slot_LogLine);
    connect(_serialConnection, &SerialConnection::configLineReceived, _configDialog, &ConfigDialog::appendConfigLine);
    connect(_serialConnection, &SerialConnection::connected, this, &MainWindow::slot_SerialConnected);
    connect(_serialConnection, &SerialConnection::disconnected, this, &MainWindow::slot_SerialDisconnected);
    connect(_serialConnection, &SerialConnection::connected, this, &MainWindow::slot_UpdateButtons);
    connect(_serialConnection, &SerialConnection::disconnected, this, &MainWindow::slot_UpdateButtons);
    connect(_serialConnection, &SerialConnection::scopePacketReceived, this, &MainWindow::slot_ScopePacketReceived);
    connect(_serialConnection, &SerialConnection::scopeResetReceived, this, &MainWindow::slot_ScopeResetReceived);
    connect(_serialConnection, &SerialConnection::errorMessage, this, &MainWindow::slot_LogError);
    connect(_jogTimer, &QTimer::timeout, this, &MainWindow::slot_SendJogCommand);
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
    _serialConnection->connectTo(_portList->currentText());
}

void MainWindow::slot_DisconnectClicked()
{
    _serialConnection->disconnectFrom(); // TODO use slot instead
}

void MainWindow::slot_EmergencyStop()
{
    _actions->driveJogEnable->setChecked(false);
    slot_DisableClicked();
}

void MainWindow::slot_DisableClicked()
{
    if (!_serialConnection->isConnected())
    {
        QMessageBox::warning(this, "Error sending reset commands", "Serial port not open!");
        return;
    }
    _serialConnection->sendData(QString("fault0.en = 0\n").toLatin1());
}

void MainWindow::slot_EnableClicked()
{
    if (!_serialConnection->isConnected())
    {
        QMessageBox::warning(this, "Error sending reset commands", "Serial port not open!");
        return;
    }
    _serialConnection->sendData(QString("fault0.en = 0\n").toLatin1());
    _serialConnection->sendData(QString("fault0.en = 1\n").toLatin1());
}

void MainWindow::slot_DataRecordToggled(bool recording)
{
    // close the old file not only when stopping, but when (re)starting
    if (_csvFile->isOpen())
        _csvFile->close();
    if (recording)
    {
        static const QString DATETIME_FORMAT = "yyyy-MM-dd_hh-mm-ss-zzz";
        const QString basePath = _recordingsDirectory.isEmpty() ? QDir::currentPath() : _recordingsDirectory; // TODO consolidate this
        const QString dateStr = QDateTime::currentDateTime().toString(DATETIME_FORMAT); // TODO use UTC version?
        static const int RETRY_COUNT = 3;
        for (int attempt = 0; !_csvFile->isOpen() && attempt < RETRY_COUNT; attempt++)
        {
            QString fileName = "data_" + dateStr;
            if (attempt > 0)
                fileName += "_" + QString::number(attempt);
            fileName += ".csv";
            const QString filePath = QDir::cleanPath(basePath + "/" + fileName);
            _csvFile->setFileName(filePath);
            _csvFile->open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::NewOnly);
        }
        if (!_csvFile->isOpen())
        {
            QMessageBox::critical(this, "Error opening recording file", "Couldn't open \"" + _csvFile->fileName() + "\" for writing!");
            _actions->dataRecord->setChecked(false);
            return;
        }
    }
}

void MainWindow::slot_DataSetDirectoryClicked()
{
    const QString dirPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"), _recordingsDirectory, QFileDialog::ShowDirsOnly);
    if (dirPath.isEmpty())
        return;
    _recordingsDirectory = dirPath;
}

void MainWindow::slot_DataOpenDirectoryClicked()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(_recordingsDirectory.isEmpty() ? QDir::currentPath() : _recordingsDirectory)); // TODO consolidate this
}

void MainWindow::slot_SendClicked()
{
    if (!_serialConnection->isConnected())
    {
        QMessageBox::warning(this, "Error sending command", "Serial port not open!");
        return;
    }
    const QString line = _lineEdit->text();
    _lineEdit->saveLine();
    _lineEdit->clear();
    _serialConnection->sendData((line + "\n").toLatin1()); // TODO perhaps have more intelligent Unicode conversion?
}

void MainWindow::slot_SerialConnected()
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">connected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

void MainWindow::slot_SerialDisconnected()
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, "<font color=\"FireBrick\">disconnected</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

void MainWindow::slot_LogLine(const QString &line)
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, line);
}

void MainWindow::slot_LogError(const QString &errorMessage)
{
    AppendTextToEdit(*_textLog, &QTextEdit::insertHtml, QString("<font color=\"FireBrick\">") + errorMessage + "</font>");
    AppendTextToEdit(*_textLog, &QTextEdit::insertPlainText, "\n");
}

void MainWindow::slot_ScopePacketReceived(const QVector<float> &packet)
{
    _oscilloscope->addChannelsSample(packet);
    _xyOscilloscope->addChannelsSample(packet);
    if (_csvFile->isOpen() && !packet.isEmpty())
    {
        QStringList fields;
        for (QVector<float>::const_iterator it = packet.begin(); it != packet.end(); ++it)
        {
            fields.append(QString::number(*it, 'f'));
        }
        _csvFile->write((fields.join(',') + "\n").toUtf8());
    }
}

void MainWindow::slot_ScopeResetReceived()
{
    _oscilloscope->resetScanning();
    _xyOscilloscope->resetScanning();
}

void MainWindow::slot_UpdateButtons()
{
    const bool portSelected = !_portList->currentText().isEmpty();
    const bool portOpen = _serialConnection->isConnected();
    const bool portClosed = _serialConnection->isDisconnected();
    const bool hasCommand = !_lineEdit->text().isEmpty();
    _portList->setEnabled(portClosed);
    _menuBar->portGroup->setEnabled(portClosed);
    _actions->connectionConnect->setEnabled(portClosed && portSelected);
    _actions->connectionDisconnect->setEnabled(!portClosed);
    _actions->viewClearConsole->setEnabled(!_textLog->document()->isEmpty());
    _actions->driveEnable->setEnabled(portOpen);
    _actions->driveDisable->setEnabled(portOpen);
    _actions->driveEditConfig->setEnabled(portOpen);
    _actions->dataRecord->setEnabled(portOpen);
    if (!portOpen)
        _actions->dataRecord->setChecked(false);
    _sendButton->setEnabled(portOpen && hasCommand);
}

void MainWindow::slot_SendJogCommand()
{
    if (!_serialConnection->isConnected())
    {
        // _jogState = JOGGING_IDLE; // TODO set to an invalid value to force sending a sychronizing command after (re)connecting
        return;
    }

    // determine new state
    JogState newState = JOGGING_IDLE;
    if (_actions->driveJogEnable->isChecked())
    {
        if (_leftPressed && !_rightPressed)
            newState = JOGGING_CCW;
        else if (_rightPressed && !_leftPressed)
            newState = JOGGING_CW;
    }

    // don't bother sending multiple "stop" commands in a row
    if (newState == JOGGING_IDLE && _jogState == JOGGING_IDLE)
        return;

    // synchronize the STMBL drive to the GUI's jog state
    _jogState = newState;
    switch (_jogState)
    {
        case JOGGING_CCW:
        _serialConnection->sendData("jogl\n");
        break;

        case JOGGING_CW:
        _serialConnection->sendData("jogr\n");
        break;

        default:
        _serialConnection->sendData("jogx\n");
    }

    // continue auto-repeating the jog command
    if (newState == JOGGING_IDLE)
        _jogTimer->stop();
    else
        _jogTimer->start();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // if (event->mimeData().hasUrls())
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
        // extract the local paths of the files
        QStringList pathList;
        {
            const QList<QUrl> urlList = mimeData->urls();
            for (int i = 0; i < urlList.size() && i < 32; ++i)
            {
                pathList.append(urlList.at(i).toLocalFile());
            }
        }

        // send the files
        QElapsedTimer timer;
        QProgressDialog progress(this);
        progress.setMinimumDuration(0);
        progress.setWindowModality(Qt::WindowModal);
        for (QStringList::const_iterator path_it = pathList.begin(); path_it != pathList.end() && !progress.wasCanceled(); ++path_it)
        {
            // get the file path
            const QString fileName = QFileInfo(*path_it).fileName();

            // read the file
            QFile file(*path_it);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QMessageBox::critical(this, "Error sending file", "Couldn't open \"" + *path_it + "\"");
                continue;
            }

            // break into lines to be sent rate-limited
            const QList<QByteArray> lines = file.readAll().split('\n');
            progress.setLabelText("Sending \"" + fileName + "\"...");
            // slot_LogLine("Sending \"" + fileName + "\"...\n");
            progress.setValue(0);
            progress.setMaximum(lines.size());
            for (QList<QByteArray>::const_iterator line_it = lines.begin(); line_it != lines.end() && !progress.wasCanceled(); ++line_it)
            {
                if (!line_it->isEmpty())
                {
                    timer.start();
                    _serialConnection->sendData(*line_it + '\n');
                    while (timer.elapsed() < 50)
                    {
                        QThread::msleep(1);
                        QCoreApplication::processEvents();
                    }
                }
                progress.setValue(progress.value()+1);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    _saveSettings();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if ((event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) && !_configDialog->isVisible())
    {
        QKeyEvent * const keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->isAutoRepeat())
            return QObject::eventFilter(obj, event);
        const bool pressed = (event->type() == QEvent::KeyPress);
        if (keyEvent->key() == Qt::Key_Left)
            _leftPressed = pressed;
        else if (keyEvent->key() == Qt::Key_Right)
            _rightPressed = pressed;
        else
            return QObject::eventFilter(obj, event);
        slot_SendJogCommand();
        return _actions->driveJogEnable->isChecked();
    }
    return QObject::eventFilter(obj, event);
}

void MainWindow::_RepopulateDeviceList()
{
    // build new list of ports
    const QStringList portNames = SerialConnection::getSerialPortNames();

    // build old list of ports
    QStringList oldPortNames;
    for (int i = 0; i < _portList->count(); i++)
    {
        oldPortNames.append(_portList->itemText(i));
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
