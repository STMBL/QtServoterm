QT += widgets serialport network
CONFIG += object_parallel_to_source

HEADERS = \
src/AppendTextToEdit.h \
src/Actions.h \
src/MenuBar.h \
src/ClickableComboBox.h \
src/ConfigDialog.h \
src/Oscilloscope.h \
src/XYOscilloscope.h \
src/HistoryLineEdit.h \
src/SerialConnection.h \
src/ScopeDataDemux.h \
src/MainWindow.h

SOURCES = \
src/Actions.cpp \
src/MenuBar.cpp \
src/ClickableComboBox.cpp \
src/ConfigDialog.cpp \
src/Oscilloscope.cpp \
src/XYOscilloscope.cpp \
src/HistoryLineEdit.cpp \
src/SerialConnection.cpp \
src/ScopeDataDemux.cpp \
src/MainWindow.cpp \
src/main.cpp

TARGET = Servoterm

CONFIG(release, debug|release) {
OBJECTS_DIR=build_release/obj
MOC_DIR=build_release/moc
}
CONFIG(debug, debug|release) {
OBJECTS_DIR=build_debug/obj
MOC_DIR=build_debug/moc
TARGET = Servoterm_debug
}

