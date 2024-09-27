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

private:
    QByteArray getPack();
    int  putInFree();
    void checkMissed();
    void processBuffer(int id);
    void setupConnection(int con_id);

    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    ScTxClient     *tx_con;
    QByteArray      tx_buf;
    ScTxClient     *dbg;
    QVector<QByteArray> read_bufs;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QVector<QTcpSocket *> rx_clients;
    QVector<QByteArray>   rx_buf;
    QVector<QTimer *>     rx_timer;
    QTimer         *refresh_timer;
    int             rx_curr_id;
    int             rc_connected; // remote client connected
    int             rx_last_id;   // last rx packed id to determine
    // if we miss a packet to request resend
};

#endif // SC_APACHE_PC_H
