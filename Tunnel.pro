TEMPLATE = app

QT += core network
CONFIG += console

MOC_DIR = Build/.moc
RCC_DIR = Build/.rcc
OBJECTS_DIR = Build/.obj
UI_DIR = Build/.ui

HEADERS += \
    Sources/backend.h \
    Sources/local.h \
    Sources/sc_apache_pc.h \
    Sources/sc_apache_pcte.h \
    Sources/sc_apache_se.h \
    Sources/sc_apache_te.h \
    Sources/sc_rx_client.h \
    Sources/sc_tx_client.h \
    Sources/sc_tx_server.h \
    Sources/sc_dbg_server.h \
    Sources/sc_dbg_client.h

win32:HEADERS +=

SOURCES += \
    Sources/local.cpp \
    Sources/main.cpp \
    Sources/sc_apache_pc.cpp \
    Sources/sc_apache_pcte.cpp \
    Sources/sc_apache_se.cpp \
    Sources/sc_apache_te.cpp \
    Sources/sc_rx_client.cpp \
    Sources/sc_tx_client.cpp \
    Sources/sc_tx_server.cpp \
    Sources/sc_dbg_server.cpp \
    Sources/sc_dbg_client.cpp

win32:SOURCES +=
