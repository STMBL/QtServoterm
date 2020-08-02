# Servoterm

Qt version of stmbl Servoterm. It is still a work in progress, stay tuned!

## Dependencies

* Qt5 (widgets and serialport modules)

The relevant development packages to install on Ubuntu 18.04 are:

* qt5-default
* libqt5serialport5-dev

## Compiling

On Linux, run the following commands in the top-level directory with the `servoterm.pro` file:

```
qmake
make
```

## Running

On Linux, you can launch the built `Servoterm` executable that will be put in the same directory as the `servoterm.pro` file:

```
./Servoterm
```
