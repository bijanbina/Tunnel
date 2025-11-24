#ifndef SC_TEST_SE_H
#define SC_TEST_SE_H

#include <stdio.h>
#include <stdlib.h>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QString>
#include <QObject>
#include <QTimer>
#include "backend.h"

class ScTestSe : public QObject
{
    Q_OBJECT

public:
    explicit ScTestSe(QObject *parent = 0);

    void init();

    QTcpSocket   *con;
    QHostAddress  ipv4;
    quint16       tx_port;

public slots:
    void clientConnected();
    void clientError();
    void txError();

    // rx
    void rxReadyRead();

    // tx
    void txTest();

private:
    QTcpServer     *server;
    QUdpSocket     *udp_con;
    QTimer         *tx_timer;
};

#endif // SC_TEST_SE_H
