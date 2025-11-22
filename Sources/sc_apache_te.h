#ifndef SC_APACHE_TE_H
#define SC_APACHE_TE_H

#include <QString>
#include <QObject>
#include <QVector>
#include <stdio.h>
#include <stdlib.h>
#include <QTimer>
#include <QSignalMapper>
#include <QUdpSocket>
#include "backend.h"
#include "sc_tx_client.h"
#include "sc_tx_server.h"

#define FA_START_PACKET "<START>\r\n"
#define FA_END_PACKET   "\r\n<END>\r\n"

class ScApacheTe : public QObject
{
    Q_OBJECT

public:
    explicit ScApacheTe(QObject *parent = 0);
    ~ScApacheTe();

    void connectApp();
    void reset();

    // rx
    QVector<QTcpSocket *> rx_cons;
    QVector<QHostAddress> rx_ipv4;

    // dbg
    QVector<QTcpSocket *> dbg_cons;
    QVector<QHostAddress> dbg_ipv4;

public slots:
    // client
    void clientConnected();
    void clientError();
    void clientDisconnected();

    // rx
    void rxReadyRead(int id);
    void rxConnected();
    void rxDisconnected(int id);
    void rxError(int id);
    QByteArray getPack();

    void txRefresh();

    // dbg
    void dbgReadyRead(int id);
    void dbgConnected();

private:
    // rx
    int  rxPutInFree();
    void rxSetupConnection(int con_id);

    // dbg
    int  dbgPutInFree();
    void dbgSetupConnection(int con_id);

    QTimer     *rx_timer;
    QTcpSocket  client;
    QByteArray  tx_buf;

    // rx
    QSignalMapper  *rx_mapper_data;
    QSignalMapper  *rx_mapper_disconnect;
    QSignalMapper  *rx_mapper_error;
    QTcpServer     *rx_server;
    int             rx_curr_id;
    QVector<QByteArray> rx_buf;
    QVector<QByteArray> read_bufs;

    // tx
    ScTxServer     *tx_server;
    QTimer         *tx_timer;
    int             tx_curr_id;

    // dbg
    QSignalMapper  *dbg_mapper_data;
    QTcpServer     *rxdbg_server;
};

#endif // SC_APACHE_TE_H
