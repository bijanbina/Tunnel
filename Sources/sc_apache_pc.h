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
#include "sc_tx_client.h"

class ScApachePC : public QObject
{
    Q_OBJECT

public:
    explicit ScApachePC(QObject *parent = 0);
    ~ScApachePC();

    void init();

    QVector<QTcpSocket *> cons;
    QVector<QHostAddress> ipv4;

signals:
    void connected(int id);
    void dataReady(int id, QString data);

public slots:
    void txReadyRead(int id);
    void clientConnected();
    void clientError(int id);
    void clientDisconnected(int id);

    // rx
    void rxReadyRead(int id);
    void rxError(int id);
    void rxDisconnected(int id);
    void rxRefresh();
    void sendAck();

    // dbg
    void dbgReadyRead(int id);
    void dbgError(int id);
    void dbgDisconnected(int id);
    void dbgRefresh();

private:
    QByteArray getPack();
    int  putInFree();
    void processBuffer(int id);
    void setupConnection(int con_id);

    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    ScTxClient     *tx_con;
    QByteArray      tx_buf;
    ScTxClient     *dbg_tx;
    QVector<QByteArray> read_bufs;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QVector<QTcpSocket *> rx_clients;
    QVector<QByteArray>   rx_buf;
    QTimer         *rx_refresh_timer;
    QTimer         *ack_timer;
    int             rx_curr_id;
    int             rc_connected; // remote client connected
                    // if we miss a packet to request resend

    // dbg rx
    QSignalMapper  *dbgrx_mapper_data;
    QSignalMapper  *dbgrx_mapper_disconnect;
    QSignalMapper  *dbgrx_mapper_error;
    QVector<QTcpSocket *> dbg_rx;
    QTimer         *dbgrx_refresh_timer;
};

#endif // SC_APACHE_PC_H
