#ifndef SC_RX_CLIENT_H
#define SC_RX_CLIENT_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QString>
#include <QObject>
#include <QVector>
#include <stdio.h>
#include <stdlib.h>
#include <QTimer>
#include <QSignalMapper>
#include "backend.h"
#include "sc_meta_client.h"
#include "sc_tx_client.h"

class ScRxClient : public QObject
{
    Q_OBJECT

public:
    explicit ScRxClient(int rx_port, QObject *parent = 0);
    ~ScRxClient();

    int  curr_id;

signals:
    void dataReady(QByteArray data);

public slots:
    void readyRead();
    void error();

private:
    QByteArray getPack();
    void processBuffer();

    QVector<QByteArray> read_bufs;

    // rx
    QUdpSocket *client;
    QByteArray  rx_buf;
    int         port;
    int         is_debug;
};

#endif // SC_APACHE_PC_H
