#include "ConfigDialog.h"
#include "SerialConnection.h"
#include "AppendTextToEdit.h"
//#include "stmbl_config_crc32.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

namespace STMBL_Servoterm {

ConfigDialog::ConfigDialog(SerialConnection *serialConnection, QWidget *parent) :
    QDialog(parent),
    _configEdit(new QPlainTextEdit),
    _saveButton(new QPushButton("Save")),
    _sizeLabel(new QLabel),
    _checksumLabel(new QLabel),
    _serialConnection(serialConnection)
{
    setWindowTitle("STMBL Configuration");
    setModal(true);
    _sizeLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _sizeLabel->setAlignment((_sizeLabel->alignment() & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight);
    _sizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    _checksumLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    _checksumLabel->setAlignment((_checksumLabel->alignment() & ~Qt::AlignHorizontal_Mask) | Qt::AlignRight);
    _checksumLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    // TODO consolidate this code
    {
        // QTextDocument * const doc = _configEdit->document();
        // QFont f = doc->defaultFont();
        QFont f = _configEdit->font();
        f.setFamily("monospace");
        f.setStyleHint(QFont::Monospace);
        // doc->setDefaultFont(f);

        _configEdit->setFont(f); // TODO this is a bit out of place, but should work just fine
        _sizeLabel->setFont(f);
        _checksumLabel->setFont(f);
    }

    QVBoxLayout * const vbox = new QVBoxLayout(this);
    vbox->addWidget(_configEdit);

    {
        QHBoxLayout * const hbox = new QHBoxLayout;
        hbox->addWidget(_saveButton);
        hbox->addWidget(_sizeLabel, 1);
        hbox->addWidget(_checksumLabel, 1);
        vbox->addLayout(hbox);
    }

    connect(_configEdit, &QPlainTextEdit::textChanged, this, &ConfigDialog::slot_ConfigTextChanged);
    connect(_saveButton, &QPushButton::clicked, this, &ConfigDialog::slot_SaveClicked);
    connect(_serialConnection, &SerialConnection::connected, this, [&] () {
        _saveButton->setEnabled(true);
    });
    connect(_serialConnection, &SerialConnection::disconnected, this, [&] () {
        _saveButton->setEnabled(false);
    });

    slot_ConfigTextChanged(); // show the initial byte count
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::appendConfigLine(const QString &configLine)
{
    AppendTextToEdit(*_configEdit, &QPlainTextEdit::insertPlainText, configLine);
}

void ConfigDialog::slot_SaveClicked()
{
    if (_serialConnection->isConnected())
    {
        _serialConnection->sendConfig(_configEdit->document()->toPlainText());
        hide();
    }
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

void ConfigDialog::slot_ConfigTextChanged()
{
    // NOTE: the -1 is to disregard an invisible
    // "paragraph separator", see the following post:
    // https://bugreports.qt.io/browse/QTBUG-4841
    _sizeLabel->setText("Size: " + QString::number(qMax(0, _configEdit->document()->characterCount()-1)).rightJustified(6) + " bytes");
    const QByteArray configBytes = _configEdit->document()->toPlainText().toLatin1();//.replace('\n', '\r');
    const quint32 checksum = CalculateCRC(configBytes);
    _checksumLabel->setText("CRC: " + QString::number(checksum, 16).rightJustified(8, '0'));
}

void ConfigDialog::showEvent(QShowEvent *event)
{
    if (_serialConnection && _serialConnection->isConnected() && !event->spontaneous())
    {
        _configEdit->clear();
        _serialConnection->startReadingConfig();
    }
}

} // namespace STMBL_Servoterm
