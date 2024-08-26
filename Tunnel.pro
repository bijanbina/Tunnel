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
    Sources/remote_client.h \
    Sources/sc_apache_pc.h \
    Sources/sc_apache_se.h \
    Sources/sc_apache_te.h

win32:HEADERS +=

SOURCES += \
    Sources/local.cpp \
    Sources/main.cpp \
    Sources/remote_client.cpp \
    Sources/sc_apache_pc.cpp \
    Sources/sc_apache_se.cpp \
    Sources/sc_apache_te.cpp

win32:SOURCES +=
