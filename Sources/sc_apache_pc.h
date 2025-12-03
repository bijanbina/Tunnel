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

    QTcpSocket   *con;

public slots:
    void txReadyRead();
    void clientConnected();
    void clientError();
    void clientDisconnected();

    // rx
    void rxReadyRead(QByteArray pack);
    void sendAck();

    // dbg
    void dbgReadyRead(QByteArray data);

private:
    void setupConnection();

    QSignalMapper *mapper_data;
    QSignalMapper *mapper_disconnect;
    QSignalMapper *mapper_error;
    QTcpServer    *server;
    ScTxClient    *tx_con;
    QByteArray     tx_buf;
    QVector<QByteArray> read_bufs;

    // rx
    ScRxClient *rx_con;
    QByteArray  rx_buf;
    QTimer     *ack_timer;

    // dbg
    ScRxClient *rx_dbg;
    int         last_ack;
    int         ack_streak;
};

#endif // SC_APACHE_PC_H
