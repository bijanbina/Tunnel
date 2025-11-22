#ifndef SC_TEST_PC_H
#define SC_TEST_PC_H

#include <stdio.h>
#include <stdlib.h>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QString>
#include <QObject>
#include <QTimer>
#include "backend.h"

class ScTestPc : public QObject
{
    Q_OBJECT

public:
    explicit ScTestPc(QObject *parent = 0);

    void init();

    QTcpSocket   *con;
    QHostAddress  ipv4;

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
    QUdpSocket     *tx_con;
    QTimer         *tx_timer;

    // rx
    QUdpSocket *rx_con;
    QTimer *    rx_timer;
};

#endif // SC_TEST_PC_H
