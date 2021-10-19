cmake_minimum_required(VERSION 3.1.0)

project(Servoterm VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets SerialPort Network REQUIRED)

add_executable(${PROJECT_NAME}
    src/Actions.cpp
    src/MenuBar.cpp
    src/ClickableComboBox.cpp
    src/ConfigDialog.cpp
    src/Oscilloscope.cpp
    src/XYOscilloscope.cpp
    src/HistoryLineEdit.cpp
    src/SerialConnection.cpp
    src/ScopeDataDemux.cpp
    src/MainWindow.cpp
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} Qt5::Widgets Qt5::SerialPort Qt5::Network)
