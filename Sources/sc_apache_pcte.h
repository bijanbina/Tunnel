#ifndef SC_APACHE_PCTE_H
#define SC_APACHE_PCTE_H

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

class ScApachePcTE : public QObject
{
    Q_OBJECT

public:
    explicit ScApachePcTE(QObject *parent = 0);
    ~ScApachePcTE();

    void init();

    QVector<QTcpSocket *> cons;
    QVector<QHostAddress> ipv4;

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

    // tx
    void txTest();

private:
    void processBuffer(int id);
    QByteArray getPack();
    int  putInFree();
    void setupConnection(int con_id);

    QSignalMapper  *mapper_data;
    QSignalMapper  *mapper_disconnect;
    QSignalMapper  *mapper_error;
    QTcpServer     *server;
    ScTxClient     *tx_con;
    ScTxClient     *dbg;
    QVector<QByteArray> read_bufs;
    QTimer         *tx_timer;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QVector<QTcpSocket *> rx_clients;
    QVector<QByteArray>   rx_buf;
    QVector<QTimer *>     rx_timer;
    QTimer         *refresh_timer;
    int             rx_curr_id;
};

#endif // SC_APACHE_PCTE_H
