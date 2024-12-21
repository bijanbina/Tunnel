#ifndef SC_APACHE_SE_H
#define SC_APACHE_SE_H

#include <QString>
#include <QObject>
#include <QVector>
#include <stdio.h>
#include <stdlib.h>
#include <QTimer>
#include <QSignalMapper>
#include <QUdpSocket>
#include "backend.h"
#include "sc_meta_server.h"
#include "sc_tx_client.h"
#include "sc_tx_server.h"

#define FA_START_PACKET "<START>\r\n"
#define FA_END_PACKET   "\r\n<END>\r\n"

class ScApacheSe : public QObject
{
    Q_OBJECT

public:
    explicit ScApacheSe(QObject *parent = 0);
    ~ScApacheSe();

    void connectApp();

    // rx
    QUdpSocket   *rx_cons;
    QHostAddress  rx_ipv4;

    QHostAddress  pc_ip;

public slots:
    // client
    void clientConnected();
    void clientError();
    void clientDisconnected();

    // rx
    void rxReadyRead();
    void rxDisconnected();;
    void rxError();
    void sendAck();

    // tx
    void txReadyRead();

    // dbg rx
    void dbgRxReadyRead();
    void dbgRxError();

private:
    void reset();

    // rx
    QByteArray getPack();

    QTimer     *rx_timer;
    QTcpSocket  client;

    // rx
    QTimer     *ack_timer;
    int         rx_curr_id;
    QByteArray  rx_buf;
    QVector<QByteArray> read_bufs;

    // tx
    ScTxServer    *tx_server;

    // dbg rx
    QUdpSocket    *dbg_rx;

    // dbg tx
    ScMetaServer  *dbg_tx;
};

#endif // SC_APACHE_SE_H
