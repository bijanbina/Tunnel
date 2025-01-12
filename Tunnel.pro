TEMPLATE = app

QT += core network
CONFIG += console

MOC_DIR = Build/.moc
RCC_DIR = Build/.rcc
OBJECTS_DIR = Build/.obj
UI_DIR = Build/.ui

HEADERS += \
    Sources/backend.h \
    Sources/dns_client.h \
    Sources/dns_server.h \
    Sources/local.h \
    Sources/sc_apache_pc.h \
    Sources/sc_apache_pcte.h \
    Sources/sc_apache_se.h \
    Sources/sc_apache_te.h \
    Sources/sc_meta_client.h \
    Sources/sc_rx_client.h \
    Sources/sc_tx_client.h \
    Sources/sc_tx_server.h

win32:HEADERS +=

SOURCES += \
    Sources/backend.cpp \
    Sources/dns_client.cpp \
    Sources/dns_server.cpp \
    Sources/local.cpp \
    Sources/main.cpp \
    Sources/sc_apache_pc.cpp \
    Sources/sc_apache_pcte.cpp \
    Sources/sc_apache_se.cpp \
    Sources/sc_apache_te.cpp \
    Sources/sc_meta_client.cpp \
    Sources/sc_rx_client.cpp \
    Sources/sc_tx_client.cpp \
    Sources/sc_tx_server.cpp

win32:SOURCES +=
