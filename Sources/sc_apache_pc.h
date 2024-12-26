#ifndef SC_APACHE_PC_H
#define SC_APACHE_PC_H

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
#include "sc_rx_client.h"
#include "sc_tx_client.h"

class ScApachePC: public QObject
{
    Q_OBJECT

public:
    explicit ScApachePC(QObject *parent = 0);
    ~ScApachePC();

    void init();
    void reset();

    QVector<QTcpSocket *> cons;
    QVector<QHostAddress> ipv4;

public slots:
    void txReadyRead(int id);
    void clientConnected();
    void clientError(int id);
    void clientDisconnected(int id);

    // rx
    void rxReadyRead(QByteArray pack);
    void sendAck();

    // dbg
    void dbgReadyRead(QByteArray data);

private:
    int  putInFree();
    void setupConnection(int con_id);

    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    ScTxClient     *tx_con;
    QByteArray      tx_buf;
    ScMetaClient   *dbg_tx;
    QVector<QByteArray> read_bufs;

    // rx
    ScRxClient *rx_con;
    QByteArray  rx_buf;
    QTimer     *ack_timer;
    int         rc_connected; // remote client connected
                    // if we miss a packet to request resend

    // dbg
    ScRxClient *dbg_rx;
};

#endif // SC_APACHE_PC_H
